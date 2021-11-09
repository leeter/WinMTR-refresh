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
#include "CWinMTRCommandLineParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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
	std::locale::global(std::locale(".UTF8"));
	winrt::init_apartment(winrt::apartment_type::single_threaded);
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
	utils::CWinMTRCommandLineParser cmd_info(mtrDialog);
	ParseCommandLine(cmd_info);
	if (cmd_info.m_help) {
		WinMTRHelp mtrHelp;
		m_pMainWnd = &mtrHelp;
		mtrHelp.DoModal();
		return FALSE;
	}
	
	m_pMainWnd = &mtrDialog;

	[[maybe_unused]]
	auto nResponse = mtrDialog.DoModal();


	return FALSE;
}

