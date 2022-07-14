module;
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <coroutine>
#include <ppltasks.h>
#include <pplawait.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <winrt/Windows.Foundation.h>

export module WinMTRDnsUtil;

export struct addrinfo_deleter final {
	void operator()(PADDRINFOEXW adder_info) const noexcept {
		FreeAddrInfoExW(adder_info);
	}
};

export auto GetAddrInfoAsync(PCWSTR pName, timeval* timeout, int family = AF_UNSPEC) noexcept {
	class name_lookup_async final {
#ifdef __has_include                           // Check if __has_include is present
#  if __has_include(<coroutine>)                // Check for a standard library
		using coro_handle = std::coroutine_handle<>;
#  else
		using coro_handle = std::experimental::coroutine_handle<>;
#  endif
#endif
		
		struct overlappedLacky final : public WSAOVERLAPPED {
			name_lookup_async* parent;
		};
		overlappedLacky lacky{ .parent{this} };
		concurrency::task_continuation_context m_context = concurrency::task_continuation_context::get_current_winrt_context();
		coro_handle m_resume{ nullptr };
		PCWSTR m_Name;
		timeval* m_timeout;
		PADDRINFOEXW m_results{ nullptr };
		DWORD m_dwError = ERROR_SUCCESS;
		int m_family;
		name_lookup_async(const name_lookup_async&) = delete;
	public:
		name_lookup_async(PCWSTR pName, timeval* timeout, int family = AF_UNSPEC) noexcept
			:m_Name(pName)
			, m_timeout(timeout)
			, m_family(family)
		{
		}

		~name_lookup_async()
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
			ADDRINFOEXW hint = { .ai_family = m_family };
			auto result = GetAddrInfoExW(
				m_Name
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

		std::optional<std::vector<SOCKADDR_STORAGE>> await_resume() const noexcept
		{
			if (m_dwError != ERROR_SUCCESS) {
				return std::nullopt;
			}

			std::vector<SOCKADDR_STORAGE> addresses;
			for (auto result = m_results; result; result = result->ai_next) {
				SOCKADDR_STORAGE addrstore = {};

				std::memcpy(&addrstore, m_results->ai_addr, m_results->ai_addrlen);
				addresses.push_back(addrstore);
			}

			return addresses;
		}

	private:
		static void
			CALLBACK lookup_callback(
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
	};
	return name_lookup_async{ pName, timeout, family };
}