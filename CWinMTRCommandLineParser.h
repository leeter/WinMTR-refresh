#pragma once
#include <afxwin.h>
class WinMTRDialog;

namespace utils {
    class CWinMTRCommandLineParser :
        public CCommandLineInfo
    {
    public:
        CWinMTRCommandLineParser(WinMTRDialog& dlg)
            :dlg(dlg){}
    private:

        void ParseParam(const WCHAR* pszParam, BOOL bFlag, BOOL bLast) override final;
#ifdef _UNICODE
        void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast) override final;
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

