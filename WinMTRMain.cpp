//*****************************************************************************
// FILE:            WinMTRMain.cpp
//
//
// HISTORY:
//
//
//    -- versions 0.8
//
// - 01.18.2002 - Store LRU hosts in registry (v0.8)
// - 05.08.2001 - Replace edit box with combo box which hold last entered hostnames.
//				  Fixed a memory leak which caused program to crash after a long 
//				  time running. (v0.7)
// - 11.27.2000 - Added resizing support and flat buttons. (v0.6) 
// - 11.26.2000 - Added copy data to clipboard and posibility to save data to file as text or HTML.(v0.5) 
// - 08.03.2000 - added double-click on hostname for detailed information (v0.4)
// - 08.02.2000 - fix icmp error codes handling. (v0.3)
// - 08.01.2000 - support for full command-line parameter specification (v0.2) 
// - 07.30.2000 - support for command-line host specification 
//					by Silviu Simen (ssimen@ubisoft.ro) (v0.1b)
// - 07.28.2000 - first release (v0.1)
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRMain.h"
#include "WinMTRDialog.h"
#include "WinMTRHelp.h"
#include <algorithm>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WinMTRMain WinMTR;

//*****************************************************************************
// BEGIN_MESSAGE_MAP
//
// 
//*****************************************************************************
BEGIN_MESSAGE_MAP(WinMTRMain, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

//*****************************************************************************
// WinMTRMain::WinMTRMain
//
// 
//*****************************************************************************
WinMTRMain::WinMTRMain()
{
}

//*****************************************************************************
// WinMTRMain::InitInstance
//
// 
//*****************************************************************************
BOOL WinMTRMain::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
	
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#endif


	WinMTRDialog mtrDialog;
	m_pMainWnd = &mtrDialog;

	if (wcslen(m_lpCmdLine)) {	
		wcscat(m_lpCmdLine,L" ");
		ParseCommandLineParams(m_lpCmdLine, &mtrDialog);
	}

	int nResponse = mtrDialog.DoModal();


	return FALSE;
}


//*****************************************************************************
// WinMTRMain::ParseCommandLineParams
//
// 
//*****************************************************************************
void WinMTRMain::ParseCommandLineParams(LPTSTR cmd, WinMTRDialog *wmtrdlg)
{
	wchar_t value[1024];

	if(GetParamValue(cmd, L"help",L'h', value)) {
		WinMTRHelp mtrHelp;
		m_pMainWnd = &mtrHelp;
		mtrHelp.DoModal();
		exit(0);
	}

	if (auto host_name = GetHostNameParamValue(cmd); host_name) {
		wmtrdlg->SetHostName(*host_name);
	}
	if(GetParamValue(cmd, L"interval",L'i', value)) {
		wchar_t* end = nullptr;
		wmtrdlg->SetInterval((float)wcstof(value, &end));
		wmtrdlg->hasIntervalFromCmdLine = true;
	}
	if(GetParamValue(cmd, L"size",L's', value)) {
		wchar_t* end = nullptr;
		wmtrdlg->SetPingSize(wcstol(value, &end, 10));
		wmtrdlg->hasPingsizeFromCmdLine = true;
	}
	if(GetParamValue(cmd, L"maxLRU",L'm', value)) {
		wchar_t* end = nullptr;
		wmtrdlg->SetMaxLRU(wcstol(value, &end, 10));
		wmtrdlg->hasMaxLRUFromCmdLine = true;
	}
	if(GetParamValue(cmd, L"numeric",L'n', value)) {
		wmtrdlg->SetUseDNS(FALSE);
		wmtrdlg->hasUseDNSFromCmdLine = true;
	}
}

//*****************************************************************************
// WinMTRMain::GetParamValue
//
// 
//*****************************************************************************
int WinMTRMain::GetParamValue(LPTSTR cmd, wchar_t * param, wchar_t sparam, wchar_t *value)
{
	using namespace std::string_view_literals;
	wchar_t *p;
	
	wchar_t p_long[1024];
	wchar_t p_short[1024];
	
	std::swprintf(p_long, std::size(p_long), L"--%s ", param);
	std::swprintf(p_short, std::size(p_short), L"-%c ", sparam);
	
	if( (p=wcsstr(cmd, p_long)) ) ;
	else 
		p=wcsstr(cmd, p_short);

	if(!p)
		return 0;

	if(L"numeric"sv == param)
		return 1;

	while(*p && *p!=L' ')
		p++;
	while(*p==L' ') p++;
	
	int i = 0;
	while(*p && *p!=L' ')
		value[i++] = *p++;
	value[i]=L'\0';

	return 1;
}

//*****************************************************************************
// WinMTRMain::GetHostNameParamValue
//
// 
//*****************************************************************************
std::optional<std::wstring> WinMTRMain::GetHostNameParamValue(LPTSTR cmd)
{
// WinMTR -h -i 1 -n google.com
	auto size = wcslen(cmd);
	std::wstring name;
	while(cmd[--size] == L' ');

	size++;
	while(size-- && cmd[size] != L' ' && (cmd[size] != L'-' || cmd[size - 1] != L' ')) {
		name = cmd[size ] + name;
	}

	if(size == -1) {
		if(name.length() == 0) {
			return std::nullopt;
		} else {
			return name;
		}
	}
	if(cmd[size] == L'-' && cmd[size - 1] == L' ') {
		// no target specified
		return std::nullopt;
	}

	std::wstring possible_argument;

	while(size-- && cmd[size] != L' ') {
		possible_argument = cmd[size] + possible_argument;
	}

	if(possible_argument.length() && (possible_argument[0] != L'-' || possible_argument == L"-n" || possible_argument == L"--numeric")) {
		return name;
	}

	return std::nullopt;
}
