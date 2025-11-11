/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2025 Leetsoftwerx

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
#include <winsock2.h>
#include <WS2tcpip.h>
#include <Ipexport.h>
module WinMTR.Net:Tracing;


#ifdef DEBUG
import <sstream>;
#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugStringW(dbg_msg.str().c_str());				\
	}
#else
#define TRACE_MSG(msg)
#endif

import <string_view>;
import <mutex>;
import <cstring>;
import <winrt/Windows.Foundation.h>;
import "WinMTRICMPPIOdef.h";
import WinMTRIPUtils;
import WinMTRICMPUtils;
import :ClassDef;

struct ICMPHandleTraits
{
	using type = HANDLE;

	static void close(type value) noexcept
	{
		WINRT_VERIFY_(TRUE, ::IcmpCloseHandle(value));
	}

	[[nodiscard]]
	static type invalid() noexcept
	{
		return INVALID_HANDLE_VALUE;
	}
};

using IcmpHandle = winrt::handle_type<ICMPHandleTraits>;

struct trace_thread final {
	trace_thread(const trace_thread&) = delete;
	trace_thread& operator=(const trace_thread&) = delete;
	trace_thread(trace_thread&&) = default;
	trace_thread& operator=(trace_thread&&) = default;

	trace_thread(ADDRESS_FAMILY af, UCHAR ttl) noexcept
		:ttl(ttl)
	{
		if (af == AF_INET) {
			icmpHandle.attach(IcmpCreateFile());
		}
		else if (af == AF_INET6) {
			icmpHandle.attach(Icmp6CreateFile());
		}
	}
	IcmpHandle icmpHandle;
	UCHAR		ttl;
};

[[nodiscard("The task should be awaited")]]
winrt::Windows::Foundation::IAsyncAction WinMTRNet::DoTrace(std::stop_token stop_token, SOCKADDR_INET address)
{
	tracing = true;
	ResetHops();
	last_remote_addr = address;

	auto threadMaker = [&address, this, stop_token](UCHAR i) {
		trace_thread current{ address.si_family, static_cast<UCHAR>(i + 1) };
		using namespace std::string_view_literals;
		TRACE_MSG(L"Thread with TTL="sv << mine.ttl << L" started."sv);
		if (address.si_family == AF_INET) {
			return this->handleICMP(address.Ipv4, stop_token, current);
		}
		else if (address.si_family == AF_INET6) {
			return this->handleICMP(address.Ipv6, stop_token, current);
		}
		winrt::throw_hresult(HRESULT_FROM_WIN32(WSAEOPNOTSUPP));
	};

	std::stop_callback callback{ stop_token, [this]() noexcept {
		this->tracing = false;
	TRACE_MSG(L"Cancellation");
		} };
	//// one thread per TTL value
	co_await std::invoke([]<UCHAR ...threads>(std::integer_sequence<UCHAR, threads...>, auto threadMaker) {
		return winrt::when_all(std::invoke(threadMaker, threads)...);
	}, std::make_integer_sequence<UCHAR, MAX_HOPS>{}, threadMaker);
	TRACE_MSG(L"Tracing Ended");
}

template<class T>
requires std::is_same_v<sockaddr_in, T> || std::is_same_v<sockaddr_in6, T>
constexpr SOCKADDR_INET to_sockaddr_inet(T addr) noexcept {
	if constexpr (std::is_same_v<sockaddr_in, T>) {
		return { .Ipv4 = addr };
	}
	else {
		return { .Ipv6 = addr };
	}
}

template<class T>
[[nodiscard("The task should be awaited")]]
winrt::Windows::Foundation::IAsyncAction WinMTRNet::handleICMP(T remote_addr, std::stop_token stop_token, trace_thread& current) {
	using namespace std::literals;
	using traits = icmp_ping_traits<T>;
	trace_thread mine = std::move(current);
	T local_addr = remote_addr;
	co_await winrt::resume_background();
	using namespace std::string_view_literals;
	const auto				nDataLen = this->options->getPingSize();
	std::vector<std::byte>	achReqData{ nDataLen, static_cast<std::byte>(32) }; //whitespaces
	std::vector<std::byte> achRepData{ reply_reply_buffer_size<T>(nDataLen) };
	winrt::handle wait_handle{ CreateEventW(nullptr, FALSE, FALSE, nullptr) };

	T* addr = reinterpret_cast<T*>(&local_addr);
	while (this->tracing) {

		// this is a backup for if the atomic above doesn't work
		if (stop_token.stop_requested()) [[unlikely]]
		{
			this->tracing = false;
			co_return;
		}
			// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
			// Check if the current thread should be closed
		if (mine.ttl > this->GetMax()) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss
		const auto dwReplyCount = co_await IcmpSendEchoAsync(mine.icmpHandle.get(), wait_handle.get(), addr, mine.ttl, achReqData, achRepData);
		this->AddXmit(mine.ttl - 1);
		if (dwReplyCount) {
			auto icmp_echo_reply = reinterpret_cast<traits::reply_type_ptr>(achRepData.data());
			TRACE_MSG(L"TTL "sv << mine.ttl << L" Status "sv << icmp_echo_reply->Status << L" Reply count "sv << dwReplyCount);

			switch (icmp_echo_reply->Status) {
			case IP_SUCCESS:
			[[likely]] case IP_TTL_EXPIRED_TRANSIT:
				this->addNewReturn(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				{
					auto naddr = traits::to_addr_from_ping(icmp_echo_reply);
					this->SetAddr(mine.ttl - 1, to_sockaddr_inet(naddr));
				}
				break;
			case IP_BUF_TOO_SMALL:
				this->SetName(mine.ttl - 1, L"Reply buffer too small."s);
				break;
			case IP_DEST_NET_UNREACHABLE:
				this->SetName(mine.ttl - 1, L"Destination network unreachable."s);
				break;
			case IP_DEST_HOST_UNREACHABLE:
				this->SetName(mine.ttl - 1, L"Destination host unreachable."s);
				break;
			case IP_DEST_PROT_UNREACHABLE:
				this->SetName(mine.ttl - 1, L"Destination protocol unreachable."s);
				break;
			case IP_DEST_PORT_UNREACHABLE:
				this->SetName(mine.ttl - 1, L"Destination port unreachable."s);
				break;
			case IP_NO_RESOURCES:
				this->SetName(mine.ttl - 1, L"Insufficient IP resources were available."s);
				break;
			case IP_BAD_OPTION:
				this->SetName(mine.ttl - 1, L"Bad IP option was specified."s);
				break;
			case IP_HW_ERROR:
				this->SetName(mine.ttl - 1, L"Hardware error occurred."s);
				break;
			case IP_PACKET_TOO_BIG:
				this->SetName(mine.ttl - 1, L"Packet was too big."s);
				break;
			case IP_REQ_TIMED_OUT:
				this->SetName(mine.ttl - 1, L"Request timed out."s);
				break;
			case IP_BAD_REQ:
				this->SetName(mine.ttl - 1, L"Bad request."s);
				break;
			case IP_BAD_ROUTE:
				this->SetName(mine.ttl - 1, L"Bad route."s);
				break;
			case IP_TTL_EXPIRED_REASSEM:
				this->SetName(mine.ttl - 1, L"The time to live expired during fragment reassembly."s);
				break;
			case IP_PARAM_PROBLEM:
				this->SetName(mine.ttl - 1, L"Parameter problem."s);
				break;
			case IP_SOURCE_QUENCH:
				this->SetName(mine.ttl - 1, L"Datagrams are arriving too fast to be processed and datagrams may have been discarded."s);
				break;
			case IP_OPTION_TOO_BIG:
				this->SetName(mine.ttl - 1, L"An IP option was too big."s);
				break;
			case IP_BAD_DESTINATION:
				this->SetName(mine.ttl - 1, L"Bad destination."s);
				break;
			case IP_GENERAL_FAILURE:
			default:
				this->SetName(mine.ttl - 1, L"General failure."s);
				break;
			}
			const auto intervalInSec = this->options->getInterval() * 1s;
			const auto roundTripDuration = std::chrono::milliseconds(icmp_echo_reply->RoundTripTime);
			if (intervalInSec > roundTripDuration) {
				using namespace winrt;
				const auto sleepTime = intervalInSec - roundTripDuration;
				co_await std::chrono::duration_cast<winrt::Windows::Foundation::TimeSpan>(sleepTime);
			}
		}

	} /* end ping loop */
	co_return;
}

winrt::fire_and_forget	WinMTRNet::SetAddr(int at, SOCKADDR_INET addr)
{
	{
		std::unique_lock lock(ghMutex);
		if (isValidAddress(host[at].addr) || !isValidAddress(addr)) {
			co_return;
		}
		host[at].addr = addr;
		//TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
	}
	if (!options->getUseDNS()) {
		co_return;
	}
	auto local_at = at;
	// this could happen after a cleanup is called, so keep this alive until the coroutine returns
	auto sharedThis = shared_from_this();
	co_await winrt::resume_background();
	wchar_t buf[NI_MAXHOST] = {};
	auto tempaddr = sharedThis->GetAddr(local_at);

	if (const auto nresult = GetNameInfoW(
		reinterpret_cast<sockaddr*>(&tempaddr)
		, static_cast<socklen_t>(getAddressSize(tempaddr))
		, buf
		, static_cast<DWORD>(std::size(buf))
		, nullptr
		, 0
		, 0);
		// zero on success
		!nresult) {
		sharedThis->SetName(local_at, buf);
	}
	else {
		sharedThis->SetName(local_at, addr_to_string(tempaddr));
	}

	TRACE_MSG(L"DNS resolver thread stopped.");
}
