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
#define WIN32_LEAN_AND_MEAN
#include <afx.h>
#include <afxext.h>
#include <afxdisp.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif 
#include "resource.h"
#include "WinMTRProperties.h"
module WinMTR.Dialog:display;
import :ClassDef;
import <format>;
import <string>;
import <string_view>;

import WinMTRVerUtil;
import WinMTRIPUtils;
import WinMTRUtils;

using namespace std::literals;

namespace {
	constexpr auto DEFAULT_PING_SIZE = 64;
	constexpr auto DEFAULT_INTERVAL = 1.0;
	constexpr auto DEFAULT_MAX_LRU = 128;
	constexpr auto DEFAULT_DNS = true;

#define MTR_NR_COLS 9
	constexpr wchar_t MTR_COLS[MTR_NR_COLS][10] = {
		L"Hostname",
		L"Nr",
		L"Loss %",
		L"Sent",
		L"Recv",
		L"Best",
		L"Avrg",
		L"Worst",
		L"Last"
	};

	constexpr int MTR_COL_LENGTH[MTR_NR_COLS] = {
			190, 30, 50, 40, 40, 50, 50, 50, 50
	};
	constexpr auto WINMTR_DIALOG_TIMER = 100;

}

//*****************************************************************************
// BEGIN_MESSAGE_MAP
//
// 
//*****************************************************************************
BEGIN_MESSAGE_MAP(WinMTRDialog, CDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_RESTART, OnRestart)
	ON_BN_CLICKED(ID_OPTIONS, OnOptions)
	ON_BN_CLICKED(ID_CTTC, OnCTTC)
	ON_BN_CLICKED(ID_CHTC, OnCHTC)
	ON_BN_CLICKED(ID_EXPT, OnEXPT)
	ON_BN_CLICKED(ID_EXPH, OnEXPH)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_MTR, OnDblclkList)
	ON_CBN_SELCHANGE(IDC_COMBO_HOST, &WinMTRDialog::OnCbnSelchangeComboHost)
	ON_CBN_SELENDOK(IDC_COMBO_HOST, &WinMTRDialog::OnCbnSelendokComboHost)
	ON_CBN_CLOSEUP(IDC_COMBO_HOST, &WinMTRDialog::OnCbnCloseupComboHost)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDCANCEL, &WinMTRDialog::OnBnClickedCancel)
END_MESSAGE_MAP()


//*****************************************************************************
// WinMTRDialog::WinMTRDialog
//
// 
//*****************************************************************************
WinMTRDialog::WinMTRDialog(CWnd* pParent) noexcept
	: CDialog(WinMTRDialog::IDD, pParent),
	interval(DEFAULT_INTERVAL),
	state(STATES::IDLE),
	transition(STATE_TRANSITIONS::IDLE_TO_IDLE),
	pingsize(DEFAULT_PING_SIZE),
	maxLRU(DEFAULT_MAX_LRU),
	useDNS(DEFAULT_DNS)

{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	wmtrnet = std::make_shared<WinMTRNet>(this);
}

//*****************************************************************************
// WinMTRDialog::DoDataExchange
//
// 
//*****************************************************************************
void WinMTRDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, ID_OPTIONS, m_buttonOptions);
	DDX_Control(pDX, IDCANCEL, m_buttonExit);
	DDX_Control(pDX, ID_RESTART, m_buttonStart);
	DDX_Control(pDX, IDC_COMBO_HOST, m_comboHost);
	DDX_Control(pDX, IDC_LIST_MTR, m_listMTR);
	DDX_Control(pDX, IDC_STATICS, m_staticS);
	DDX_Control(pDX, IDC_STATICJ, m_staticJ);
	DDX_Control(pDX, ID_EXPH, m_buttonExpH);
	DDX_Control(pDX, ID_EXPT, m_buttonExpT);
}


//*****************************************************************************
// WinMTRDialog::OnInitDialog
//
// 
//*****************************************************************************
BOOL WinMTRDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	const auto verNumber = WinMTRVerUtil::getExeVersion();
#ifndef  _WIN64
	constexpr auto bitness = 32;
#else
	constexpr auto bitness = 64;
#endif
	const auto caption = std::format(L"WinMTR-Refresh v{} {} bit"sv, verNumber, bitness);
	SetTimer(1, WINMTR_DIALOG_TIMER, nullptr);
	SetWindowTextW(caption.c_str());

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	if (!statusBar.Create(this))
		AfxMessageBox(L"Error creating status bar");
	statusBar.GetStatusBarCtrl().SetMinHeight(23);

	UINT sbi[1] = { IDS_STRING_SB_NAME };
	statusBar.SetIndicators(sbi);
	statusBar.SetPaneInfo(0, statusBar.GetItemID(0), SBPS_STRETCH, 0);
	// removing for now but leaving commenteded this goes to a domain buying site, so either they lost the domain or are
	// out of business. I'll fix this when that changes.
	//{ // Add appnor URL
	//	//std::unique_ptr<CMFCLinkCtrl> m_pWndButton = std::make_unique<CMFCLinkCtrl>();
	//	if (!m_pWndButton.Create(_T("www.appnor.com"), WS_CHILD|WS_VISIBLE|WS_TABSTOP, CRect(0,0,0,0), &statusBar, 1234)) {
	//		TRACE(_T("Failed to create button control.\n"));
	//		return FALSE;
	//	}

	//	m_pWndButton.SetURL(L"http://www.appnor.com/?utm_source=winmtr&utm_medium=desktop&utm_campaign=software");
	//		
	//	if(!statusBar.AddPane(1234,1)) {
	//		AfxMessageBox(_T("Pane index out of range\nor pane with same ID already exists in the status bar"), MB_ICONERROR);
	//		return FALSE;
	//	}
	//		
	//	statusBar.SetPaneWidth(statusBar.CommandToIndex(1234), 100);
	//	statusBar.AddPaneControl(&m_pWndButton, 1234, true);
	//}

	for (int i = 0; i < MTR_NR_COLS; i++) {
		m_listMTR.InsertColumn(i, MTR_COLS[i], LVCFMT_LEFT, MTR_COL_LENGTH[i], -1);
	}

	m_comboHost.SetFocus();

	// We need to resize the dialog to make room for control bars.
	// First, figure out how big the control bars are.
	CRect rcClientStart;
	CRect rcClientNow;
	GetClientRect(rcClientStart);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,
		0, reposQuery, rcClientNow);

	// Now move all the controls so they are in the same relative
	// position within the remaining client area as they would be
	// with no control bars.
	//CPoint ptOffset(rcClientNow.left - rcClientStart.left,
					//rcClientNow.top - rcClientStart.top);
	const auto ptOffset = rcClientNow.TopLeft() - rcClientStart.TopLeft();

	CRect  rcChild;
	CWnd* pwndChild = GetWindow(GW_CHILD);
	while (pwndChild)
	{
		pwndChild->GetWindowRect(rcChild);
		ScreenToClient(rcChild);
		rcChild.OffsetRect(ptOffset);
		pwndChild->MoveWindow(rcChild, FALSE);
		pwndChild = pwndChild->GetNextWindow();
	}

	// Adjust the dialog window dimensions
	CRect rcWindow;
	GetWindowRect(rcWindow);
	rcWindow.right += rcClientStart.Width() - rcClientNow.Width();
	rcWindow.bottom += rcClientStart.Height() - rcClientNow.Height();
	MoveWindow(rcWindow, FALSE);

	// And position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	InitRegistry();

	if (m_autostart) {
		m_comboHost.SetWindowText(msz_defaulthostname.c_str());
		OnRestart();
	}

	return FALSE;
}

//*****************************************************************************
// WinMTRDialog::OnSizing
//
// 
//*****************************************************************************
void WinMTRDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	int iWidth = (pRect->right) - (pRect->left);
	int iHeight = (pRect->bottom) - (pRect->top);

	if (iWidth < 600)
		pRect->right = pRect->left + 600;
	if (iHeight < 250)
		pRect->bottom = pRect->top + 250;
}


//*****************************************************************************
// WinMTRDialog::OnSize
//
// 
//*****************************************************************************
void WinMTRDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	CRect r;
	GetClientRect(&r);
	CRect lb;
	if (::IsWindow(m_staticS.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_staticS.m_hWnd);
		m_staticS.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(10, dpi, 96);
		m_staticS.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width() - lb.TopLeft().x - scaledXOffset, lb.Height(), SWP_NOMOVE | SWP_NOZORDER);
	}

	if (::IsWindow(m_staticJ.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_staticJ.m_hWnd);
		m_staticJ.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(21, dpi, 96);
		m_staticJ.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width() - scaledXOffset, lb.Height(), SWP_NOMOVE | SWP_NOZORDER);
	}

	if (::IsWindow(m_buttonExit.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_buttonExit.m_hWnd);
		m_buttonExit.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(21, dpi, 96);
		m_buttonExit.SetWindowPos(nullptr, r.Width() - lb.Width() - scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height(), SWP_NOSIZE | SWP_NOZORDER);
	}

	if (::IsWindow(m_buttonExpH.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_buttonExpH.m_hWnd);
		m_buttonExpH.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(21, dpi, 96);
		m_buttonExpH.SetWindowPos(nullptr, r.Width() - lb.Width() - scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height(), SWP_NOSIZE | SWP_NOZORDER);
	}
	if (::IsWindow(m_buttonExpT.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_buttonExpT.m_hWnd);
		m_buttonExpT.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(103, dpi, 96);
		m_buttonExpT.SetWindowPos(nullptr, r.Width() - lb.Width() - scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height(), SWP_NOSIZE | SWP_NOZORDER);
	}

	if (::IsWindow(m_listMTR.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_listMTR.m_hWnd);
		m_listMTR.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledX = MulDiv(21, dpi, 96);
		const auto scaledY = MulDiv(25, dpi, 96);
		m_listMTR.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width() - scaledX, r.Height() - lb.top - scaledY, SWP_NOMOVE | SWP_NOZORDER);
	}

	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,
		0, reposQuery, r);

	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

}


//*****************************************************************************
// WinMTRDialog::OnPaint
//
// 
//*****************************************************************************
void WinMTRDialog::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
		const auto dpi = GetDpiForWindow(*this);
		const int cxIcon = GetSystemMetricsForDpi(SM_CXICON, dpi);
		const int cyIcon = GetSystemMetricsForDpi(SM_CYICON, dpi);
		CRect rect;
		GetClientRect(&rect);
		const int x = (rect.Width() - cxIcon + 1) / 2;
		const int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}


//*****************************************************************************
// WinMTRDialog::OnQueryDragIcon
//
// 
//*****************************************************************************
HCURSOR WinMTRDialog::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}


//*****************************************************************************
// WinMTRDialog::OnDblclkList
//
//*****************************************************************************
void WinMTRDialog::OnDblclkList([[maybe_unused]] NMHDR* pNMHDR, LRESULT* pResult)
{
	using namespace std::string_view_literals;
	*pResult = 0;

	if (state == STATES::TRACING) {

		POSITION pos = m_listMTR.GetFirstSelectedItemPosition();
		if (pos != nullptr) {
			int nItem = m_listMTR.GetNextSelectedItem(pos);
			WinMTRProperties wmtrprop;

			if (const auto lstate = wmtrnet->getStateAt(nItem); !isValidAddress(lstate.addr)) {
				wmtrprop.host.clear();
				wmtrprop.ip.clear();
				wmtrprop.comment = lstate.getName();

				wmtrprop.pck_loss = wmtrprop.pck_sent = wmtrprop.pck_recv = 0;

				wmtrprop.ping_avrg = wmtrprop.ping_last = 0.0;
				wmtrprop.ping_best = wmtrprop.ping_worst = 0.0;
			}
			else {
				wmtrprop.host = lstate.getName();
				wmtrprop.ip = addr_to_string(lstate.addr);

				wmtrprop.comment = L"Host alive."sv;

				wmtrprop.ping_avrg = static_cast<float>(lstate.getAvg());
				wmtrprop.ping_last = static_cast<float>(lstate.last);
				wmtrprop.ping_best = static_cast<float>(lstate.best);
				wmtrprop.ping_worst = static_cast<float>(lstate.worst);

				wmtrprop.pck_loss = lstate.getPercent();
				wmtrprop.pck_recv = lstate.returned;
				wmtrprop.pck_sent = lstate.xmit;
			}

			wmtrprop.DoModal();
		}
	}
}


//*****************************************************************************
// WinMTRDialog::SetHostName
//
//*****************************************************************************
void WinMTRDialog::SetHostName(std::wstring host)
{
	m_autostart = true;
	msz_defaulthostname = std::move(host);
}


//*****************************************************************************
// WinMTRDialog::SetPingSize
//
//*****************************************************************************
void WinMTRDialog::SetPingSize(unsigned ps, options_source fromCmdLine) noexcept
{
	pingsize = ps;
	hasPingsizeFromCmdLine = static_cast<bool>(fromCmdLine);
}

//*****************************************************************************
// WinMTRDialog::SetMaxLRU
//
//*****************************************************************************
void WinMTRDialog::SetMaxLRU(int mlru, options_source fromCmdLine) noexcept
{
	maxLRU = mlru;
	hasMaxLRUFromCmdLine = static_cast<bool>(fromCmdLine);
}


//*****************************************************************************
// WinMTRDialog::SetInterval
//
//*****************************************************************************
void WinMTRDialog::SetInterval(float i, options_source fromCmdLine) noexcept
{
	interval = i;
	hasMaxLRUFromCmdLine = static_cast<bool>(fromCmdLine);
}

//*****************************************************************************
// WinMTRDialog::SetUseDNS
//
//*****************************************************************************
void WinMTRDialog::SetUseDNS(bool udns, options_source fromCmdLine) noexcept
{
	useDNS = udns;
	hasUseDNSFromCmdLine = static_cast<bool>(fromCmdLine);
}


//*****************************************************************************
// WinMTRDialog::WinMTRDialog
//
// 
//*****************************************************************************
void WinMTRDialog::OnCancel()
{
}


//*****************************************************************************
// WinMTRDialog::DisplayRedraw
//
// 
//*****************************************************************************
int WinMTRDialog::DisplayRedraw()
{
	wchar_t buf[255] = {}, nr_crt[255] = {};
	const auto netstate = wmtrnet->getCurrentState();
	const auto nh = netstate.size();
	while (m_listMTR.GetItemCount() > nh) {
		m_listMTR.DeleteItem(m_listMTR.GetItemCount() - 1);
	}

	static CString noResponse((LPCWSTR)IDS_STRING_NO_RESPONSE_FROM_HOST);

	for (int i = 0; const auto & host : netstate) {

		auto name = host.getName();
		if (name.empty()) {
			name = noResponse;
		}

		auto result = std::format_to_n(nr_crt, std::size(nr_crt) - 1, WinMTRUtils::int_number_format, i + 1);
		*result.out = '\0';
		if (m_listMTR.GetItemCount() <= i)
			m_listMTR.InsertItem(i, name.c_str());
		else
			m_listMTR.SetItem(i, 0, LVIF_TEXT, name.c_str(), 0, 0, 0, 0);

		m_listMTR.SetItem(i, 1, LVIF_TEXT, nr_crt, 0, 0, 0, 0);
		constexpr auto writable_size = std::size(buf) - 1;
		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.getPercent());
		*result.out = '\0';
		m_listMTR.SetItem(i, 2, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.xmit);
		*result.out = '\0';
		m_listMTR.SetItem(i, 3, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.returned);
		*result.out = '\0';
		m_listMTR.SetItem(i, 4, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.best);
		*result.out = '\0';
		m_listMTR.SetItem(i, 5, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.getAvg());
		*result.out = '\0';
		m_listMTR.SetItem(i, 6, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.worst);
		*result.out = '\0';
		m_listMTR.SetItem(i, 7, LVIF_TEXT, buf, 0, 0, 0, 0);

		result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, host.last);
		*result.out = '\0';
		m_listMTR.SetItem(i, 8, LVIF_TEXT, buf, 0, 0, 0, 0);

		i++;
	}

	return 0;
}

void WinMTRDialog::OnCbnSelchangeComboHost()
{
}

void WinMTRDialog::OnCbnSelendokComboHost()
{
}


void WinMTRDialog::OnCbnCloseupComboHost()
{
	if (m_comboHost.GetCurSel() == m_comboHost.GetCount() - 1) {
		ClearHistory();
	}
}