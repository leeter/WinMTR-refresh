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
#include <WinSock2.h>
export module WinMTRSNetHost;

import WinMTRIPUtils;
import <string>;

export struct s_nethost final {
	SOCKADDR_STORAGE addr = {};
	std::wstring name;
	int xmit = 0;			// number of PING packets sent
	int returned = 0;		// number of ICMP echo replies received
	unsigned long total = 0;	// total time
	int last = 0;				// last time
	int best = 0;				// best time
	int worst = 0;			// worst time
	[[nodiscard]]
	inline auto getPercent() const noexcept {
		return (xmit == 0) ? 0 : (100 - (100 * returned / xmit));
	}
	[[nodiscard]]
	inline int getAvg() const noexcept {
		return returned == 0 ? 0 : total / returned;
	}
	[[nodiscard]]
	std::wstring getName() const {
		if (name.empty()) {
			return addr_to_string(addr);
		}
		return name;
	}
};
