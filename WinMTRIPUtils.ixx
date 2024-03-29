/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2022 Leetsoftwerx

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
#include <Ws2ipdef.h>
export module WinMTRIPUtils;

import <type_traits>;
import <concepts>;
import <string>;

export template<class T>
concept socket_type = requires(T a) {
	a.sa_family;
	requires std::convertible_to<decltype(a.sa_family), ADDRESS_FAMILY>;
};

template<typename T>
	requires socket_type<T> || std::convertible_to<T, SOCKADDR_STORAGE> || std::convertible_to<T, SOCKADDR_INET>
inline constexpr auto getAddressFamily(const T & addr) noexcept {
	using cvref_t = std::remove_cvref_t<T>;
	if constexpr (socket_type<T>) {
		return addr.sa_family;
	}
	else if constexpr (std::is_same_v<SOCKADDR_INET, cvref_t>) {
		return addr.si_family;
	}
	else if (std::is_same_v<SOCKADDR_STORAGE, cvref_t>) {
		return addr.ss_family;
	}
}

export template<typename T>
inline constexpr size_t getAddressSize(const T& addr) noexcept {
	return getAddressFamily(addr) == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

export template<class T>
concept socket_addr_type = socket_type<T> || std::convertible_to<T, sockaddr_storage> || std::convertible_to<T, SOCKADDR_INET>;

export template<socket_addr_type T>
inline constexpr bool isValidAddress(const T& addr) noexcept {
	const ADDRESS_FAMILY family = getAddressFamily(addr);

	return (family == AF_INET || family == AF_INET6);
}

export template<socket_addr_type T>
[[nodiscard]]
auto addr_to_string(const T & addr) noexcept -> std::wstring {
	if (!isValidAddress(addr)) {
		return {};
	}
	// remove const at cost of a copy
	std::remove_cv_t<T> laddr = addr;
	std::wstring out;
	out.resize(40); // max IPv6 Address length + 1 for terminator
	const auto addrlen = getAddressSize(addr);
	// mutated by WSAAddressToStringW
	DWORD addrstrsize = static_cast<DWORD>(out.size());
	if (const auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&laddr), static_cast<DWORD>(addrlen), nullptr, out.data(), &addrstrsize); !result && addrstrsize > 1) {
		out.resize(addrstrsize - 1);
	}
	return out;
}
