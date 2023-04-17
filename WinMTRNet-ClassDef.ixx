/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2023 Leetsoftwerx

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
export module WinMTR.Net:ClassDef;

import <optional>;
import <atomic>;
import <array>;
import <mutex>;
import <memory>;
import <stop_token>;
import <winrt/base.h>;
import <winrt/Windows.Foundation.h>;
import WinMTRSNetHost;
import WinMTROptionsProvider;
import winmtr.helper;

struct trace_thread;

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

	static constexpr auto MAX_HOPS = 30;
private:
	std::array<s_nethost, WinMTRNet::MAX_HOPS>	host;
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
	winrt::fire_and_forget	SetAddr(int at, sockaddr& addr);
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
	winrt::Windows::Foundation::IAsyncAction handleICMP(std::stop_token stop_token, trace_thread& current);
};