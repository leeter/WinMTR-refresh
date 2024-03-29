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

export module WinMTRUtils;
import <string_view>;
using namespace std::literals;
export namespace WinMTRUtils {
	export constexpr auto int_number_format = L"{:Ld}"sv;
	export constexpr auto float_number_format = L"{:.1Lf}"sv;
	export constexpr auto DEFAULT_PING_SIZE = 64u;
	export constexpr auto MAX_PING_SIZE = 1u << 15u;
	export constexpr auto MIN_PING_SIZE = DEFAULT_PING_SIZE;
	export constexpr auto DEFAULT_INTERVAL = 1.0;
	export constexpr auto MIN_INTERVAL = DEFAULT_INTERVAL;
	export constexpr auto MAX_INTERVAL = 120.0;
	export constexpr auto DEFAULT_MAX_LRU = 128u;
	export constexpr auto MIN_MAX_LRU = 1u;
	export constexpr auto MAX_MAX_LRU = 1024u;
}