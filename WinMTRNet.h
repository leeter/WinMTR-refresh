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
// FILE:            WinMTRNet.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************
#pragma once
#ifndef WINMTRNET_H_
#define WINMTRNET_H_
#include <string>
#include <atomic>
#include <mutex>
#include <array>
#include <vector>
#include <concepts>
#include <type_traits>
#include <winrt/Windows.Foundation.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include "IWinMTROptionsProvider.hpp"
import winmtr.helper;

constexpr auto MAX_HOPS = 30;

struct s_nethost {
  SOCKADDR_STORAGE addr = {};
  std::wstring name;
  int xmit = 0;			// number of PING packets sent
  int returned = 0;		// number of ICMP echo replies received
  unsigned long total = 0;	// total time
  int last = 0;				// last time
  int best = 0;				// best time
  int worst = 0;			// worst time
  inline auto getPercent() const noexcept {
	  return (xmit == 0) ? 0 : (100 - (100 * returned / xmit));
  }
  inline int getAvg() const noexcept {
	  return returned == 0 ? 0 : total / returned;
  }
  std::wstring getName() const;
};

template<class T>
concept socket_type = requires(T a) {
	a.sa_family;
	std::convertible_to<decltype(a.sa_family), ADDRESS_FAMILY>;
};

template<typename T>
requires socket_type<T> || std::convertible_to<T, SOCKADDR_STORAGE>
inline constexpr auto getAddressFamily(const T& addr) noexcept {
	if constexpr (socket_type<T>) {
		return addr.sa_family;
	}
	else if (std::is_same_v<SOCKADDR_STORAGE, T>) {
		return addr.ss_family;
	}
}

template<typename T>
inline constexpr size_t getAddressSize(const T& addr) noexcept {
	return getAddressFamily(addr) == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

template<class T>
concept socket_addr_type = socket_type<T> || std::convertible_to<T, sockaddr_storage>;

template<socket_addr_type T>
inline constexpr bool isValidAddress(const T& addr) noexcept {
	const ADDRESS_FAMILY family = getAddressFamily(addr);
	
	return (family == AF_INET || family == AF_INET6);
}
struct trace_thread;

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

class WinMTRNet final: public std::enable_shared_from_this<WinMTRNet>{
	WinMTRNet(const WinMTRNet&) = delete;
	WinMTRNet& operator=(const WinMTRNet&) = delete;
public:

	WinMTRNet(const IWinMTROptionsProvider*wp);
	~WinMTRNet() noexcept;
	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction	DoTrace(sockaddr& address);
	void	ResetHops() noexcept;

	[[nodiscard]]
	int		GetMax() const;
	[[nodiscard]]
	std::vector<s_nethost> getCurrentState() const;
	s_nethost getStateAt(int at) const;
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
	SOCKADDR_STORAGE GetAddr(int at) const;
	void	SetAddr(int at, sockaddr& addr);
	void	SetName(int at, std::wstring n);
	void addNewReturn(int ttl, int last);
	void	AddXmit(int at);

	template<class T>
	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction handleICMP(trace_thread current);
};

#endif	// ifndef WINMTRNET_H_
