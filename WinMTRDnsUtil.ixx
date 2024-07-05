module;
#pragma warning (disable : 4005)
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMCX
#define NOIME
#define NOGDI
#define NONLS
#define NOAPISET
#define NOSERVICE
#define NOMINMAX
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <ws2ipdef.h>
export module WinMTRDnsUtil;

import <string_view>;
import <optional>;
import <expected>;
import <coroutine>;
import <type_traits>;
import <cstring>;
import <string>;
import <ppltasks.h>;
import <pplawait.h>;
import <winrt/Windows.Foundation.h>;


export template<class T, class U>
concept layout_compatible = std::is_layout_compatible_v<U, T>;

export template<layout_compatible<ADDRINFOEXW> A>
std::optional<SOCKADDR_INET> get_sockaddr_from_addrinfo(const A* info) {
	if (info->ai_addr == nullptr) {
		return std::nullopt;
	}
	SOCKADDR_INET addrstore = {};

	std::memcpy(&addrstore, info->ai_addr, info->ai_addrlen);
	return addrstore;
}

//export template<layout_compatible<ADDRINFOEXW> A>
//std::optional<std::wstring> get_name_from_addrinfo(const ADDRINFOEXW* info) {
//	if (info->ai_canonname == nullptr) {
//		return std::nullopt;
//	}
//	
//	return std::wstring(info->ai_canonname);
//}

export struct addrinfo_deleter final {
	void operator()(PADDRINFOEXW adder_info) const noexcept {
		FreeAddrInfoExW(adder_info);
	}
};

class name_lookup_async final {
	using coro_handle = std::coroutine_handle<>;

	struct overlappedLacky final : public WSAOVERLAPPED {
		name_lookup_async* parent;
	};
	overlappedLacky lacky{ .parent{this} };
	concurrency::task_continuation_context m_context = concurrency::task_continuation_context::get_current_winrt_context();
	coro_handle m_resume{ nullptr };
	const std::wstring_view m_Name;
	timeval* m_timeout;
	PADDRINFOEXW m_results{ nullptr };
	DWORD m_dwError = ERROR_SUCCESS;
	int m_family;
	int m_flags;
	name_lookup_async(const name_lookup_async&) = delete;
public:
	name_lookup_async(const std::wstring_view pName, timeval* timeout, int family = AF_UNSPEC, int flags = 0) noexcept
		:m_Name(pName)
		, m_timeout(timeout)
		, m_family(family)
		, m_flags(flags)
	{
	}

	~name_lookup_async() noexcept
	{
		if (m_results) {
			FreeAddrInfoExW(m_results);
		}

	}

	bool await_ready() const noexcept
	{
		return false;
	}

	void await_suspend(coro_handle resume_handle)
	{
		m_resume = resume_handle;
		// AF_UNSPEC counts as AF_INET | AF_INET6
		ADDRINFOEXW hint = { .ai_flags = m_flags, .ai_family = m_family };
		const auto result = GetAddrInfoExW(
			m_Name.data()
			, nullptr ///PCWSTR                             pServiceName,
			, NS_DNS
			, nullptr //LPGUID                             lpNspId,
			, &hint
			, &m_results // PADDRINFOEXW * ppResult,
			, m_timeout
			, &lacky
			, name_lookup_async::lookup_callback
			, nullptr //LPHANDLE                           lpHandle
		);

		if (result != WSA_IO_PENDING) {
			winrt::throw_hresult(HRESULT_FROM_WIN32(result));
		}
	}

	std::expected<std::vector<SOCKADDR_INET>, winrt::hresult> await_resume() const noexcept
	{
		if (m_dwError != ERROR_SUCCESS) {
			return std::unexpected{ winrt::hresult{HRESULT_FROM_WIN32(m_dwError) } };
		}

		std::vector<SOCKADDR_INET> addresses;
		for (auto result = m_results; result; result = result->ai_next) {
			if (const auto addr = get_sockaddr_from_addrinfo(result); addr) {
				addresses.push_back(*addr);
			}
		}

		return addresses;
	}

private:
	static void
		CALLBACK lookup_callback(
			__in      DWORD    dwError,
			__in [[maybe_unused]] DWORD    dwBytes,
			__in      LPWSAOVERLAPPED lpOverlapped
		) noexcept;
};

export auto GetAddrInfoAsync(const std::wstring_view pName, timeval* timeout, int family = AF_UNSPEC, int flags = 0) noexcept {
	
	return name_lookup_async{ pName, timeout, family, flags };
}
module: private;

void
CALLBACK name_lookup_async::lookup_callback(
	__in      DWORD    dwError,
	__in [[maybe_unused]] DWORD    dwBytes,
	__in      LPWSAOVERLAPPED lpOverlapped
) noexcept {
	auto context = static_cast<overlappedLacky*>(lpOverlapped)->parent;
	context->m_dwError = dwError;
	concurrency::create_task([=] {
		context->m_resume();
		}, context->m_context);
}