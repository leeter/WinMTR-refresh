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

module;
#pragma warning (disable : 4005)
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
//#include "WinMTRGlobal.h"
#include <afxwin.h>
#include <format>
#include "resource.h"
//#include "WinMTROptions.h"
export module WinMTR.Options;
import <iterator>;
import WinMTRUtils;

import WinMTR.License;




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//*****************************************************************************
// CLASS:  WinMTROptions
//
//
//*****************************************************************************
export class WinMTROptions final : public CDialog
{
public:
	WinMTROptions(CWnd* pParent = nullptr);

	inline void SetUseDNS(bool udns) noexcept { useDNS = udns; };
	inline void SetInterval(double i) noexcept { interval = i; };
	inline void SetPingSize(int ps) noexcept { pingsize = ps; };
	inline void SetMaxLRU(int mlru) noexcept { maxLRU = mlru; };
	inline void SetUseIPv4(bool uip4) noexcept { useIPv4 = uip4; }
	inline void SetUseIPv6(bool uip6) noexcept { useIPv6 = uip6; }

	inline auto GetInterval() const noexcept { return interval; };
	inline auto GetPingSize() const noexcept { return pingsize; };
	inline auto GetMaxLRU()	const noexcept { return maxLRU; };
	inline auto GetUseDNS() const noexcept { return useDNS; }
	inline auto GetUseIPv4() const noexcept { return useIPv4; }
	inline auto GetUseIPv6() const noexcept { return useIPv6; }

	enum { IDD = IDD_DIALOG_OPTIONS };
	CEdit	m_editSize;
	CEdit	m_editInterval;
	CEdit	m_editMaxLRU;
	CButton	m_checkDNS;
	CButton m_useIPv4;
	CButton m_useIPv6;

protected:
	void DoDataExchange(CDataExchange* pDX) override;

	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnLicense();
	DECLARE_MESSAGE_MAP()

private:
	double   interval = 0.0;
	int      pingsize = 0;
	int		 maxLRU = 0;
	bool     useDNS = false;
	bool	 useIPv4 = true;
	bool	 useIPv6 = true;

public:
	afx_msg void OnBnClickedIpv4Check();
	afx_msg void OnBnClickedUseipv6Check();
};

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
	
	wchar_t strtmp[20] = {};
	constexpr auto writable_size = std::size(strtmp) - 1;
	
	auto result = std::format_to_n(strtmp, writable_size, WinMTRUtils::float_number_format, interval);
	*result.out = '\0';
	m_editInterval.SetWindowText(strtmp);

	result = std::format_to_n(strtmp, writable_size, WinMTRUtils::int_number_format, pingsize);
	*result.out = '\0';
	m_editSize.SetWindowText(strtmp);
	
	result = std::format_to_n(strtmp, writable_size, WinMTRUtils::int_number_format, maxLRU);
	*result.out = '\0';
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
