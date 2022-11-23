/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2021 Leetsoftwerx

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
#include "WinMTRICMPPIOdef.h"
#include <iphlpapi.h>
#include <icmpapi.h>
export module WinMTRICMPUtils;

import <concepts>;
import <coroutine>;
import <span>;
import <type_traits>;
import <ppl.h>;
import <ppltasks.h>;
import <pplawait.h>;

constexpr auto ECHO_REPLY_TIMEOUT = 5000;

export
template<class T>
struct ping_reply {
	using type = void;
};

export
template<>
struct ping_reply<sockaddr_in> {
	using type = ICMP_ECHO_REPLY;
};

export
template<>
struct ping_reply<sockaddr_in6> {
	using type = ICMPV6_ECHO_REPLY;
};

export
template <class T>
using ping_reply_t = ping_reply<T>::type;

export
template<class T>
struct any_address {
	static constexpr void value() noexcept {};
};

export
template<>
struct any_address<sockaddr_in> {
	static constexpr auto value() noexcept {
		return ADDR_ANY;
	}
};

export
template<>
struct any_address<sockaddr_in6> {
	static SOCKADDR_IN6* value() noexcept {
		static sockaddr_in6 anyaddr = { .sin6_family = AF_INET6, .sin6_addr = in6addr_any };
		return &anyaddr;
	}
};

export
template<class T>
concept icmp_type = requires {
	typename T::reply_type;
	typename T::reply_type_ptr;
	typename T::addrtype;
	typename T::storagetype;
};

export
template<class T>
concept supports_ICMP_ping = icmp_type<T> && requires(
	_In_                       HANDLE                   IcmpHandle,
	_In_opt_                   HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
	_In_opt_                   PIO_APC_ROUTINE          ApcRoutine,
#else
	_In_opt_                   FARPROC                  ApcRoutine,
#endif
	_In_opt_                   PVOID                    ApcContext,
	_In_                       typename T::addrtype                   SourceAddress,
	_In_                       typename T::addrtype                   DestinationAddress,
	_In_reads_bytes_(RequestSize)   LPVOID                   RequestData,
	_In_                       WORD                     RequestSize,
	_In_opt_                   PIP_OPTION_INFORMATION   RequestOptions,
	_Out_writes_bytes_(ReplySize)    LPVOID                   ReplyBuffer,
	_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
	DWORD                    ReplySize,
	_In_                       DWORD                    Timeout) {
		{ T::pingmethod(IcmpHandle,
			Event,
			ApcRoutine,
			ApcContext,
			SourceAddress,
			DestinationAddress,
			RequestData,
			RequestSize,
			RequestOptions,
			ReplyBuffer,
			ReplySize,
			Timeout) } noexcept -> std::convertible_to<DWORD>;
};

export
template<class T>
concept supports_ICMP_ping_parse = icmp_type<T> && requires(
	_Out_writes_bytes_(ReplySize)   LPVOID                   ReplyBuffer,
	_In_range_(> , sizeof(T::reply_type) + 8)
	DWORD                    ReplySize) {
		{
			T::parsemethod(
				ReplyBuffer,
				ReplySize)  } noexcept -> std::convertible_to<DWORD>;
};

export
template<class T>
concept icmp_pingable = icmp_type<T> && supports_ICMP_ping<T> && supports_ICMP_ping_parse<T> && requires(
	std::add_const_t<typename T::reply_type_ptr> rtp
	, std::add_pointer_t<typename T::storagetype> storage) {
		{ T::get_anyaddr() } noexcept -> std::convertible_to<typename T::addrtype>;
		{ T::to_addr_from_ping(rtp) } noexcept -> std::convertible_to<typename T::storagetype>;
		{ T::to_addr_from_storage(storage) } noexcept -> std::convertible_to<typename T::addrtype>;
};

export
template<typename T>
struct icmp_ping_traits {
	using reply_type = ping_reply_t<T>;
	using reply_type_ptr = std::add_pointer_t<reply_type>;
	using addrtype = void;
	using storagetype = void;
	static constexpr auto pingmethod = 0;
	static constexpr auto parsemethod = 0;
	static auto get_anyaddr() noexcept ->  addrtype {
		return;
	}
	static storagetype to_addr_from_ping(const reply_type_ptr reply) noexcept {
		return {};
	}
};

export
template<>
struct icmp_ping_traits<sockaddr_in> {
	using reply_type = ping_reply_t<sockaddr_in>;
	using reply_type_ptr = std::add_pointer_t<reply_type>;
	using addrtype = IPAddr;
	using storagetype = sockaddr_in;
	static inline DWORD
		WINAPI
		pingmethod(
			_In_                       HANDLE                   IcmpHandle,
			_In_opt_                   HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
			_In_opt_                   PIO_APC_ROUTINE          ApcRoutine,
#else
			_In_opt_                   FARPROC                  ApcRoutine,
#endif
			_In_opt_                   PVOID                    ApcContext,
			_In_                       addrtype                   SourceAddress,
			_In_                       addrtype                   DestinationAddress,
			_In_reads_bytes_(RequestSize)   LPVOID                   RequestData,
			_In_                       WORD                     RequestSize,
			_In_opt_                   PIP_OPTION_INFORMATION   RequestOptions,
			_Out_writes_bytes_(ReplySize)    LPVOID                   ReplyBuffer,
			_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
			DWORD                    ReplySize,
			_In_                       DWORD                    Timeout
		) noexcept {
		return IcmpSendEcho2Ex(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress, RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);
	}
	static inline DWORD
		WINAPI
		parsemethod(
			_Out_writes_bytes_(ReplySize)   LPVOID                   ReplyBuffer,
			_In_range_(> , sizeof(reply_type) + 8)
			DWORD                    ReplySize
		) noexcept {
		return IcmpParseReplies(ReplyBuffer, ReplySize);
	}
	static  auto get_anyaddr() noexcept -> addrtype {
		return any_address<sockaddr_in>::value();
	}

	static addrtype to_addr_from_storage(storagetype* storage) noexcept {
		return storage->sin_addr.S_un.S_addr;
	}

	static storagetype to_addr_from_ping(const reply_type_ptr reply) noexcept {
		sockaddr_in naddr = { .sin_family = AF_INET };
		naddr.sin_addr.S_un.S_addr = reply->Address;
		return naddr;
	}
};

export
template<>
struct icmp_ping_traits<sockaddr_in6> {
	using reply_type = ping_reply_t<sockaddr_in6>;
	using reply_type_ptr = std::add_pointer_t<reply_type>;
	using addrtype = sockaddr_in6*;
	using storagetype = std::remove_pointer_t<addrtype>;
	static inline DWORD
		WINAPI
		pingmethod(
			_In_                       HANDLE                   IcmpHandle,
			_In_opt_                   HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
			_In_opt_                   PIO_APC_ROUTINE          ApcRoutine,
#else
			_In_opt_                   FARPROC                  ApcRoutine,
#endif
			_In_opt_                   PVOID                    ApcContext,
			_In_                       addrtype                   SourceAddress,
			_In_                       addrtype                   DestinationAddress,
			_In_reads_bytes_(RequestSize)   LPVOID                   RequestData,
			_In_                       WORD                     RequestSize,
			_In_opt_                   PIP_OPTION_INFORMATION   RequestOptions,
			_Out_writes_bytes_(ReplySize)    LPVOID                   ReplyBuffer,
			_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
			DWORD                    ReplySize,
			_In_                       DWORD                    Timeout
		) noexcept {
		return Icmp6SendEcho2(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress, RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);
	}
	static inline DWORD
		WINAPI
		parsemethod(
			_Out_writes_bytes_(ReplySize)   LPVOID                   ReplyBuffer,
			_In_range_(> , sizeof(reply_type) + 8)
			DWORD                    ReplySize
		) noexcept {
		return Icmp6ParseReplies(ReplyBuffer, ReplySize);
	}
	static auto get_anyaddr() noexcept -> addrtype {
		return any_address<sockaddr_in6>::value();
	}

	static constexpr addrtype to_addr_from_storage(storagetype* storage) noexcept {
		return storage;
	}
	static storagetype to_addr_from_ping(const reply_type_ptr reply) noexcept {
		sockaddr_in6 naddr = { .sin6_family = AF_INET6 };
		memcpy(&naddr.sin6_addr, reply->Address.sin6_addr, sizeof(naddr.sin6_addr));
		return naddr;
	}
};

export
template<icmp_pingable traits>
struct icmp_ping final {
	using coro_handle = std::coroutine_handle<>;
	icmp_ping(HANDLE icmpHandle, traits::addrtype addr, UCHAR ttl, std::span<std::byte> requestData, std::span<std::byte> replyData) noexcept
		:m_reqData(requestData)
		, m_replyData(replyData)
		, m_handle(icmpHandle)
		, m_addr(addr)
		, m_ttl(ttl)
	{

	}

	void await_suspend(coro_handle resume_handle)
	{
		m_resume = resume_handle;
		IP_OPTION_INFORMATION	stIPInfo = {
			.Ttl = m_ttl,
			.Flags = IP_FLAG_DF
		};
		auto local = traits::get_anyaddr();

		const auto io_res = traits::pingmethod(
			m_handle
			, nullptr
			, &callback
			, this
			, local
			, m_addr
			, m_reqData.data()
			, static_cast<WORD>(m_reqData.size())
			, &stIPInfo
			, m_replyData.data()
			, static_cast<DWORD>(m_replyData.size())
			, ECHO_REPLY_TIMEOUT);

		if (io_res != ERROR_SUCCESS) [[unlikely]] {
			throw std::system_error(io_res, std::system_category());
		}
		if (const auto err = GetLastError(); err != ERROR_IO_PENDING) [[unlikely]] {
			throw std::system_error(err, std::system_category());
		}
	}

	auto await_resume() const noexcept
	{
		return traits::parsemethod(m_replyData.data(), m_replysize);
	}

	bool await_ready() const noexcept
	{
		return false;
	}
private:
	static void NTAPI callback(IN PVOID ApcContext,
		IN PIO_STATUS_BLOCK IoStatusBlock,
		IN ULONG Reserved) noexcept {
		UNREFERENCED_PARAMETER(Reserved);
		auto context = static_cast<icmp_ping<traits>*>(ApcContext);
		context->m_replysize = static_cast<DWORD>(IoStatusBlock->Information);
		concurrency::create_task([=]() noexcept {
			context->m_resume();
			}, context->m_context);
	}

	concurrency::task_continuation_context m_context = concurrency::task_continuation_context::get_current_winrt_context();
	coro_handle m_resume{ nullptr };
	std::span<std::byte> m_reqData;
	std::span<std::byte> m_replyData;
	HANDLE m_handle;
	traits::addrtype m_addr;
	DWORD m_replysize = 0;
	UCHAR m_ttl;
};


export
template<class T, class addrtype = std::remove_pointer_t<std::remove_cvref_t<T>>, class traits = icmp_ping_traits<addrtype>>
inline auto IcmpSendEchoAsync(HANDLE icmpHandle, T addr, UCHAR ttl, std::span<std::byte> requestData, std::span<std::byte> replyData) noexcept {
	return icmp_ping<traits>{icmpHandle, traits::to_addr_from_storage(addr), ttl, requestData, replyData};
}

export
template<class T>
[[nodiscard]]
constexpr auto reply_reply_buffer_size(unsigned requestSize) noexcept {
	return sizeof(ping_reply_t<T>) + 8 + sizeof(IO_STATUS_BLOCK) + requestSize;
}
