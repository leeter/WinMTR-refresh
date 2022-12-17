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


//*****************************************************************************
// FILE:            WinMTRNet.cpp
//
//*****************************************************************************
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
#include "resource.h"
#include <icmpapi.h>
export module WinMTR.Net;

import <string>;
import <atomic>;
import <mutex>;
import <array>;
import <memory>;
import <vector>;
import <optional>;
import <winrt/Windows.Foundation.h>;
import <stop_token>;
import WinMTROptionsProvider;
import winmtr.helper;

import WinMTRIPUtils;
import WinMTRSNetHost;

#ifdef DEBUG
#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugStringW(dbg_msg.str().c_str());				\
	}
#else
#define TRACE_MSG(msg)
#endif

constexpr auto MAX_HOPS = 30;

class WinMTRNet;
namespace {

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

	[[nodiscard]]
	inline bool operator==(const SOCKADDR_STORAGE& lhs, const SOCKADDR_STORAGE& rhs) noexcept {
		return std::memcmp(&lhs, &rhs, sizeof(SOCKADDR_STORAGE)) == 0;
	}
	
	struct trace_thread final {
		trace_thread(const trace_thread&) = delete;
		trace_thread& operator=(const trace_thread&) = delete;
		trace_thread(trace_thread&&) = default;
		trace_thread& operator=(trace_thread&&) = default;

		trace_thread(ADDRESS_FAMILY af, WinMTRNet* winmtr, UCHAR ttl) noexcept
			:
			address(),
			winmtr(winmtr),
			ttl(ttl)
		{
			if (af == AF_INET) {
				icmpHandle.attach(IcmpCreateFile());
			}
			else if (af == AF_INET6) {
				icmpHandle.attach(Icmp6CreateFile());
			}
		}
		SOCKADDR_STORAGE address;
		IcmpHandle icmpHandle;
		WinMTRNet* winmtr;
		UCHAR		ttl;
	};
}

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

export class WinMTRNet final : public std::enable_shared_from_this<WinMTRNet> {
	WinMTRNet(const WinMTRNet&) = delete;
	WinMTRNet& operator=(const WinMTRNet&) = delete;
public:

	WinMTRNet(const IWinMTROptionsProvider* wp)
		:host(),
		last_remote_addr(),
		options(wp),
		wsaHelper(MAKEWORD(2, 2)),
		tracing() {

		if (!wsaHelper) [[unlikely]] {
			//AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
			return;
		}
	}
	~WinMTRNet() noexcept = default;


	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction	DoTrace(std::stop_token stop_token, sockaddr& address);

	void	ResetHops() noexcept
	{
		for (auto& h : this->host) {
			h = s_nethost();
		}
	}
	[[nodiscard]]
	int		GetMax() const;

	[[nodiscard]]
	std::vector<s_nethost> getCurrentState() const;
	s_nethost getStateAt(int at) const
	{
		std::unique_lock lock(ghMutex);
		return host[at];
	}
private:
	std::array<s_nethost, MAX_HOPS>	host;
	SOCKADDR_STORAGE last_remote_addr;
	mutable std::recursive_mutex	ghMutex;
	std::optional<winrt::Windows::Foundation::IAsyncAction> tracer;
	std::optional<winrt::apartment_context> context;
	const IWinMTROptionsProvider* options;
	winmtr::helper::WSAHelper wsaHelper;
	std::atomic_bool	tracing;

	[[nodiscard]]
	SOCKADDR_STORAGE GetAddr(int at) const
	{
		std::unique_lock lock(ghMutex);
		return host[at].addr;
	}
	void	SetAddr(int at, sockaddr& addr);
	void	SetName(int at, std::wstring n)
	{
		std::unique_lock lock(ghMutex);
		host[at].name = std::move(n);
	}

	void addNewReturn(int at, int last)
	{
		std::unique_lock lock(ghMutex);
		host[at].last = last;
		host[at].total += last;
		if (host[at].best > last || host[at].xmit == 1) {
			host[at].best = last;
		};
		if (host[at].worst < last) {
			host[at].worst = last;
		}
		host[at].returned++;
	}
	void	AddXmit(int at)
	{
		std::unique_lock lock(ghMutex);
		host[at].xmit++;
	}

	template<class T>
	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction handleICMP(std::stop_token stop_token, trace_thread current);
};

module : private;

import <sstream>;
import <string_view>;
import <type_traits>;
import <cstring>;
import <iterator>;
import <ppltasks.h>;
import WinMTRICMPUtils;

[[nodiscard]]
std::vector<s_nethost> WinMTRNet::getCurrentState() const
{
	std::unique_lock lock(ghMutex);
	auto max = GetMax();
	auto end = std::cbegin(host);
	std::advance(end, max);
	return std::vector<s_nethost>(std::cbegin(host), end);
}

[[nodiscard]]
int WinMTRNet::GetMax() const
{
	std::unique_lock lock(ghMutex);
	int max = MAX_HOPS;

	// first match: traced address responds on ping requests, and the address is in the hosts list
	for (int i = 1; const auto & h : host) {
		if (h.addr == last_remote_addr) {
			max = i;
			break;
		}
		++i;
	}

	// second match:  traced address doesn't responds on ping requests
	if (max == MAX_HOPS) {
		while ((max > 1) && (host[max - 1].addr == host[max - 2].addr && isValidAddress(host[max - 1].addr))) max--;
	}
	return max;
}

[[nodiscard("The task should be awaited")]]
winrt::Windows::Foundation::IAsyncAction WinMTRNet::DoTrace(std::stop_token stop_token, sockaddr& address)
{
	tracing = true;
	ResetHops();
	last_remote_addr = {};
	std::memcpy(&last_remote_addr, &address, getAddressSize(address));

	auto threadMaker = [&address, this, stop_token](UCHAR i) {
		trace_thread current(address.sa_family, this, i + 1);
		using namespace std::string_view_literals;
		TRACE_MSG(L"Thread with TTL="sv << current.ttl << L" started."sv);
		std::memcpy(&current.address, &address, getAddressSize(address));
		if (current.address.ss_family == AF_INET) {
			return this->handleICMP<sockaddr_in>(stop_token, std::move(current));
		}
		else if (current.address.ss_family == AF_INET6) {
			return this->handleICMP<sockaddr_in6>(stop_token, std::move(current));
		}
		winrt::throw_hresult(HRESULT_FROM_WIN32(WSAEOPNOTSUPP));
	};

	std::stop_callback callback(stop_token, [this]() noexcept {
		this->tracing = false;
	TRACE_MSG(L"Cancellation");
		});
	//// one thread per TTL value
	co_await std::invoke([]<UCHAR ...threads>(std::integer_sequence<UCHAR, threads...>, auto threadMaker) {
		return winrt::when_all(std::invoke(threadMaker, threads)...);
	}, std::make_integer_sequence<UCHAR, MAX_HOPS>(), threadMaker);
	TRACE_MSG(L"Tracing Ended");
}

template<class T>
[[nodiscard("The task should be awaited")]]
winrt::Windows::Foundation::IAsyncAction WinMTRNet::handleICMP(std::stop_token stop_token, trace_thread current) {
	using namespace std::literals;
	using traits = icmp_ping_traits<T>;
	trace_thread mine = std::move(current);
	co_await winrt::resume_background();
	using namespace std::string_view_literals;
	const auto				nDataLen = this->options->getPingSize();
	std::vector<std::byte>	achReqData(nDataLen, static_cast<std::byte>(32)); //whitespaces
	std::vector<std::byte> achRepData(reply_reply_buffer_size<T>(nDataLen));

	
	T* addr = reinterpret_cast<T*>(&mine.address);
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
		const auto dwReplyCount = co_await IcmpSendEchoAsync(mine.icmpHandle.get(), addr, mine.ttl, achReqData, achRepData);
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
					this->SetAddr(mine.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
				}
				break;
			case IP_BUF_TOO_SMALL:
				this->SetName(current.ttl - 1, L"Reply buffer too small."s);
				break;
			case IP_DEST_NET_UNREACHABLE:
				this->SetName(current.ttl - 1, L"Destination network unreachable."s);
				break;
			case IP_DEST_HOST_UNREACHABLE:
				this->SetName(current.ttl - 1, L"Destination host unreachable."s);
				break;
			case IP_DEST_PROT_UNREACHABLE:
				this->SetName(current.ttl - 1, L"Destination protocol unreachable."s);
				break;
			case IP_DEST_PORT_UNREACHABLE:
				this->SetName(current.ttl - 1, L"Destination port unreachable."s);
				break;
			case IP_NO_RESOURCES:
				this->SetName(current.ttl - 1, L"Insufficient IP resources were available."s);
				break;
			case IP_BAD_OPTION:
				this->SetName(current.ttl - 1, L"Bad IP option was specified."s);
				break;
			case IP_HW_ERROR:
				this->SetName(current.ttl - 1, L"Hardware error occurred."s);
				break;
			case IP_PACKET_TOO_BIG:
				this->SetName(current.ttl - 1, L"Packet was too big."s);
				break;
			case IP_REQ_TIMED_OUT:
				this->SetName(current.ttl - 1, L"Request timed out."s);
				break;
			case IP_BAD_REQ:
				this->SetName(current.ttl - 1, L"Bad request."s);
				break;
			case IP_BAD_ROUTE:
				this->SetName(current.ttl - 1, L"Bad route."s);
				break;
			case IP_TTL_EXPIRED_REASSEM:
				this->SetName(current.ttl - 1, L"The time to live expired during fragment reassembly."s);
				break;
			case IP_PARAM_PROBLEM:
				this->SetName(current.ttl - 1, L"Parameter problem."s);
				break;
			case IP_SOURCE_QUENCH:
				this->SetName(current.ttl - 1, L"Datagrams are arriving too fast to be processed and datagrams may have been discarded."s);
				break;
			case IP_OPTION_TOO_BIG:
				this->SetName(current.ttl - 1, L"An IP option was too big."s);
				break;
			case IP_BAD_DESTINATION:
				this->SetName(current.ttl - 1, L"Bad destination."s);
				break;
			case IP_GENERAL_FAILURE:
			default:
				this->SetName(current.ttl - 1, L"General failure."s);
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

void	WinMTRNet::SetAddr(int at, sockaddr& addr)
{
	std::unique_lock lock(ghMutex);
	if (!isValidAddress(host[at].addr) && isValidAddress(addr)) {
		host[at].addr = {};
		//TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
		memcpy(&host[at].addr, &addr, getAddressSize(addr));

		if (options->getUseDNS()) {
			Concurrency::create_task([at, sharedThis = shared_from_this()]() noexcept {
				TRACE_MSG(L"DNS resolver thread started.");

			wchar_t buf[NI_MAXHOST] = {};
			sockaddr_storage addr = sharedThis->GetAddr(at);

			if (const auto nresult = GetNameInfoW(
				reinterpret_cast<sockaddr*>(&addr)
				, static_cast<socklen_t>(getAddressSize(addr))
				, buf
				, static_cast<DWORD>(std::size(buf))
				, nullptr
				, 0
				, 0);
				// zero on success
				!nresult) {
				sharedThis->SetName(at, buf);
			}
			else {
				sharedThis->SetName(at, addr_to_string(addr));
			}

			TRACE_MSG(L"DNS resolver thread stopped.");
				});
		}
	}
}

