#include "WinMTRGlobal.h"
#include "CWinMTRCommandLineParser.h"
#include "WinMTRDialog.h"


namespace utils {

	void CWinMTRCommandLineParser::ParseParam(const WCHAR* pszParam, BOOL bFlag, BOOL bLast)
	{
		UNREFERENCED_PARAMETER(bLast);
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

	void CWinMTRCommandLineParser::ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
	{
		using namespace std::literals;
	}

}
