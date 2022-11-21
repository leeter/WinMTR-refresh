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

#pragma once
#include <afxwin.h>
class WinMTRDialog;

namespace utils {
    class CWinMTRCommandLineParser final :
        public CCommandLineInfo
    {
    public:
        CWinMTRCommandLineParser(WinMTRDialog& dlg) noexcept
            :dlg(dlg){}
    private:

        void ParseParam(const WCHAR* pszParam, BOOL bFlag, BOOL bLast) noexcept override final;
#ifdef _UNICODE
        void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast) noexcept override final;
#endif
    private:
        
        WinMTRDialog& dlg;

        enum class expect_next {
            none,
            interval,
            ping_size,
            lru
        };
        expect_next next = expect_next::none;

    public:

        bool m_help = false;
    };
}

