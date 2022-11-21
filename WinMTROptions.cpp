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
// FILE:            WinMTROptions.cpp
//
//
//*****************************************************************************

#include "WinMTRGlobal.h"
import <iterator>;
#include "WinMTROptions.h"
#include "WinMTRLicense.h"

#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//*****************************************************************************
// BEGIN_MESSAGE_MAP
//
// 
//*****************************************************************************
BEGIN_MESSAGE_MAP(WinMTROptions, CDialog)
	ON_BN_CLICKED(ID_LICENSE, OnLicense)
	ON_BN_CLICKED(IDC_IPV4_CHECK, &WinMTROptions::OnBnClickedIpv4Check)
	ON_BN_CLICKED(IDC_USEIPV6_CHECK, &WinMTROptions::OnBnClickedUseipv6Check)
END_MESSAGE_MAP()


//*****************************************************************************
// WinMTROptions::WinMTROptions
//
// 
//*****************************************************************************
WinMTROptions::WinMTROptions(CWnd* pParent) : CDialog(WinMTROptions::IDD, pParent)
{
}


//*****************************************************************************
// WinMTROptions::DoDataExchange
//
// 
//*****************************************************************************
void WinMTROptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SIZE, m_editSize);
	DDX_Control(pDX, IDC_EDIT_INTERVAL, m_editInterval);
	DDX_Control(pDX, IDC_EDIT_MAX_LRU, m_editMaxLRU);
	DDX_Control(pDX, IDC_CHECK_DNS, m_checkDNS);
	DDX_Control(pDX, IDC_USEIPV6_CHECK, m_useIPv6);
	DDX_Control(pDX, IDC_IPV4_CHECK, m_useIPv4);
}


//*****************************************************************************
// WinMTROptions::OnInitDialog
//
// 
//*****************************************************************************
BOOL WinMTROptions::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	wchar_t strtmp[20];
	
	std::swprintf(strtmp, std::size(strtmp), L"%.1f", interval);
	m_editInterval.SetWindowText(strtmp);
	
	std::swprintf(strtmp, std::size(strtmp), L"%d", pingsize);
	m_editSize.SetWindowText(strtmp);
	
	std::swprintf(strtmp, std::size(strtmp), L"%d", maxLRU);
	m_editMaxLRU.SetWindowText(strtmp);

	m_checkDNS.SetCheck(useDNS);
	m_useIPv4.SetCheck(useIPv4);
	m_useIPv6.SetCheck(useIPv6);
	
	m_editInterval.SetFocus();
	return FALSE;
}


//*****************************************************************************
// WinMTROptions::OnOK
//
// 
//*****************************************************************************
void WinMTROptions::OnOK() 
{
	wchar_t tmpstr[20];
	
	useDNS = m_checkDNS.GetCheck();

	m_editInterval.GetWindowText(tmpstr, 20);
	wchar_t* end = nullptr;
	interval = wcstod(tmpstr, &end);

	m_editSize.GetWindowText(tmpstr, 20);
	end = nullptr;
	pingsize = wcstol(tmpstr, &end, 10);
	
	m_editMaxLRU.GetWindowText(tmpstr, 20);
	end = nullptr;
	maxLRU = wcstol(tmpstr, &end, 10);

	useIPv4 = m_useIPv4.GetCheck();
	useIPv6 = m_useIPv6.GetCheck();

	CDialog::OnOK();
}

//*****************************************************************************
// WinMTROptions::OnLicense
//
// 
//*****************************************************************************
void WinMTROptions::OnLicense() 
{
	WinMTRLicense mtrlicense;
	mtrlicense.DoModal();
}



void WinMTROptions::OnBnClickedIpv4Check()
{
	
	if (!m_useIPv4.GetCheck()) {
		m_useIPv6.SetCheck(true);
	}
}


void WinMTROptions::OnBnClickedUseipv6Check()
{
	
	if (!m_useIPv6.GetCheck()) {
		m_useIPv4.SetCheck(true);
	}
}
