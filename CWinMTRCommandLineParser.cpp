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

#include "WinMTRGlobal.h"
#include "CWinMTRCommandLineParser.h"
#include "WinMTRDialog.h"


namespace utils {

	void CWinMTRCommandLineParser::ParseParam(const WCHAR* pszParam, BOOL bFlag, [[maybe_unused]] BOOL bLast) noexcept
	{
		using namespace std::literals;
		if (bFlag) {
			if (L"h"sv == pszParam || L"-help"sv == pszParam) {
				this->m_help = true;
			}
			else if (L"n"sv == pszParam || L"-numeric"sv == pszParam) {
				this->dlg.SetUseDNS(false, WinMTRDialog::options_source::cmd_line);
			}
			else if (L"i"sv == pszParam || L"-interval"sv == pszParam) {
				this->next = expect_next::interval;
			}
			else if (L"m"sv == pszParam || L"-maxLRU"sv == pszParam) {
				this->next = expect_next::lru;
			}
			else if (L"s"sv == pszParam || L"-size"sv == pszParam) {
				this->next = expect_next::ping_size;
			}
			return;
		}
		wchar_t* end = nullptr;
		switch (this->next) {
		case expect_next::lru:
			this->dlg.SetMaxLRU(std::wcstol(pszParam, &end, 10), WinMTRDialog::options_source::cmd_line);
			break;
		case expect_next::interval:
			this->dlg.SetInterval(static_cast<float>(std::wcstof(pszParam, &end)), WinMTRDialog::options_source::cmd_line);
			break;
		case expect_next::ping_size:
			this->dlg.SetPingSize(std::wcstol(pszParam, &end, 10), WinMTRDialog::options_source::cmd_line);
			break;
		default:
			break;
		}
		this->next = expect_next::none;
	}

	void CWinMTRCommandLineParser::ParseParam([[maybe_unused]] const char* pszParam, [[maybe_unused]] BOOL bFlag, [[maybe_unused]] BOOL bLast) noexcept
	{
		using namespace std::literals;
	}

}
