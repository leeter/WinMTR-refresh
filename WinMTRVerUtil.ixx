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
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module WinMTRVerUtil;

import <string>;

export namespace WinMTRVerUtil {
	const std::wstring& getExeVersion();
}

module : private;

import <vector>;
import <format>;
import <string_view>;

using namespace std::literals;

namespace {
	struct verInfo {
		std::wstring versionStr;
		verInfo() {
			WCHAR modulepath[MAX_PATH + 1]{};
			if (!GetModuleFileNameW(nullptr, modulepath, std::size(modulepath))) {
				return;
			}
			DWORD legacy;
			DWORD versize;
			if (versize = GetFileVersionInfoSizeExW(FILE_VER_GET_LOCALISED, modulepath, &legacy); versize == 0) {
				return;
			}

			std::vector<char> buffer(static_cast<std::size_t>(versize));

			if (!GetFileVersionInfoExW(FILE_VER_GET_LOCALISED, modulepath, 0, versize, buffer.data())) [[unlikely]] {
				return;
			}
			LPVOID subblock = nullptr;
			UINT len = 0;
			if (!VerQueryValueW(buffer.data(), L"\\", &subblock, &len)) [[unlikely]] {
				return;
			}

			if (!len) [[unlikely]] {
				return;
			}

			const VS_FIXEDFILEINFO* verInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(subblock);
			if (verInfo->dwSignature != 0xfeef04bd)
			{
				return;
			}
			const int major = HIWORD(verInfo->dwProductVersionMS);
			const int minor = LOWORD(verInfo->dwProductVersionMS);
			const int revision = HIWORD(verInfo->dwProductVersionLS);
			// not currently in use, maybe in the future
			//const int build = LOWORD(verInfo->dwProductVersionLS);

			versionStr = std::format(L"{}.{}{}"sv, major, minor, revision);
		}
	};
}

const std::wstring& WinMTRVerUtil::getExeVersion() {
	// YAY magic statics making this thread safe!
	static verInfo info;
	return info.versionStr;
}