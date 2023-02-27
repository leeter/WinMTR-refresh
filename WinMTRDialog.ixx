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
// FILE:            WinMTRDialog.cpp
//
//
//*****************************************************************************
module;
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
//#include <afxdtctl.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif 
#pragma warning (disable : 4005)
#include "resource.h"
#include "WinMTRProperties.h"
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.Diagnostics.h>
#include <ws2tcpip.h>
export module WinMTR.Dialog;

import <string>;
import <winrt/Windows.Foundation.h>;
import <memory>;
import <mutex>;
import <optional>;
import <atomic>;
import <thread>;
import WinMTROptionsProvider;
import WinMTRStatusBar;
import WinMTR.Net;
import WinMTR.Options;

using namespace std::string_view_literals;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static	 char THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugStringW(dbg_msg.str().c_str());				\
	}
#else
#define TRACE_MSG(msg)
#endif

constexpr auto WINMTR_DIALOG_TIMER = 100;



//*****************************************************************************
// CLASS:  WinMTRDialog
//
//
//*****************************************************************************

export class WinMTRDialog final : public CDialog, public IWinMTROptionsProvider
{
public:
	WinMTRDialog(CWnd* pParent = nullptr) noexcept;

	enum { IDD = IDD_WINMTR_DIALOG };
	enum class options_source : bool {
		none,
		cmd_line
	};

	afx_msg BOOL InitRegistry() noexcept;

	WinMTRStatusBar	statusBar;

	enum class STATES {
		IDLE = 0,
		TRACING,
		STOPPING,
		EXIT
	};

	enum class STATE_TRANSITIONS {
		IDLE_TO_IDLE = 0,
		IDLE_TO_TRACING,
		IDLE_TO_EXIT,
		TRACING_TO_TRACING,
		TRACING_TO_STOPPING,
		TRACING_TO_EXIT,
		STOPPING_TO_IDLE,
		STOPPING_TO_STOPPING,
		STOPPING_TO_EXIT
	};



	bool InitMTRNet() noexcept;

	int DisplayRedraw();
	void Transit(STATES new_state);

private:
	CButton	m_buttonOptions;
	CButton	m_buttonExit;
	CButton	m_buttonStart;
	CComboBox m_comboHost;
	CListCtrl	m_listMTR;

	CStatic	m_staticS;
	CStatic	m_staticJ;

	CButton	m_buttonExpT;
	CButton	m_buttonExpH;
	std::wstring msz_defaulthostname;
	std::shared_ptr<WinMTRNet>			wmtrnet;
	std::mutex tracer_mutex;
	std::optional<std::jthread> trace_lacky;
	HICON m_hIcon;
	double				interval;
	STATES				state;
	STATE_TRANSITIONS	transition;
	unsigned			pingsize;
	int					maxLRU;
	int					nrLRU = 0;
	bool				m_autostart = false;
	bool				hasPingsizeFromCmdLine = false;
	bool				hasMaxLRUFromCmdLine = false;
	bool				hasIntervalFromCmdLine = false;
	bool				useDNS;
	bool				hasUseDNSFromCmdLine = false;
	bool				useIPv4 = true;
	bool				useIPv6 = true;
	std::atomic_bool	tracing;

	void ClearHistory();
	winrt::Windows::Foundation::IAsyncAction pingThread(std::stop_token token, std::wstring shost);
	winrt::fire_and_forget stopTrace();
public:

	void SetHostName(std::wstring host);
	void SetInterval(float i, options_source fromCmdLine = options_source::none) noexcept;
	void SetPingSize(unsigned ps, options_source fromCmdLine = options_source::none) noexcept;
	void SetMaxLRU(int mlru, options_source fromCmdLine = options_source::none) noexcept;
	void SetUseDNS(bool udns, options_source fromCmdLine = options_source::none) noexcept;

	inline double getInterval() const noexcept { return interval; }
	inline unsigned getPingSize() const noexcept { return pingsize; }
	inline bool getUseDNS() const noexcept { return useDNS; }

protected:
	void DoDataExchange(CDataExchange* pDX) override;

	BOOL OnInitDialog() override;
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT, int, int);
	afx_msg void OnSizing(UINT, LPRECT);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRestart() noexcept;
	afx_msg void OnOptions();
	void OnCancel() override;

	afx_msg void OnCTTC() noexcept;
	afx_msg void OnCHTC() noexcept;
	afx_msg void OnEXPT() noexcept;
	afx_msg void OnEXPH() noexcept;

	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeComboHost();
	afx_msg void OnCbnSelendokComboHost();
	afx_msg void OnCbnCloseupComboHost();
	afx_msg void OnTimer(UINT_PTR nIDEvent) noexcept;
	afx_msg void OnClose();
	afx_msg void OnBnClickedCancel();
};

module : private;

import <cstring>;
import <sstream>;
import <iterator>;
import <string_view>;
import <fstream>;
import <format>;
import <stop_token>;
import WinMTRIPUtils;
import WinMTRSNetHost;
import WinMTRDnsUtil;
import WinMTRUtils;
import WinMTRVerUtil;

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
	const auto config_key_name = LR"(Software\WinMTR\Config)";
	const auto lru_key_name = LR"(Software\WinMTR\LRU)";	
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
	
	if(!statusBar.Create( this ))
		AfxMessageBox(L"Error creating status bar");
	statusBar.GetStatusBarCtrl().SetMinHeight(23);
		
	UINT sbi[1] = {IDS_STRING_SB_NAME};
	statusBar.SetIndicators(sbi);
	statusBar.SetPaneInfo(0, statusBar.GetItemID(0),SBPS_STRETCH, 0 );
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


static constexpr auto reg_host_fmt = L"Host{:d}"sv;
const auto NrLRU_REG_KEY = L"NrLRU";
//*****************************************************************************
// WinMTRDialog::InitRegistry
//
// 
//*****************************************************************************
BOOL WinMTRDialog::InitRegistry() noexcept
{
	CRegKey versionKey;
	if (versionKey.Create(HKEY_CURRENT_USER,
		LR"(Software\WinMTR)",
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS) != ERROR_SUCCESS){
		return FALSE;

	}
	static const auto WINMTR_VERSION = L"0.96";
	static const auto WINMTR_LICENSE = L"GPL - GNU Public License";
	static const auto WINMTR_HOMEPAGE = L"https://github.com/leeter/WinMTR-refresh";
	versionKey.SetStringValue(L"Version", WinMTRVerUtil::getExeVersion().c_str());
	versionKey.SetStringValue(L"License", WINMTR_LICENSE);
	versionKey.SetStringValue(L"HomePage", WINMTR_HOMEPAGE);
	CRegKey config_key;
	if (config_key.Create(versionKey,
		L"Config",
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS) != ERROR_SUCCESS) {
		return FALSE;
	}
	DWORD tmp_dword;
	if(config_key.QueryDWORDValue(L"PingSize", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = pingsize;
		config_key.SetDWORDValue(L"PingSize", tmp_dword);
	} else {
		if(!hasPingsizeFromCmdLine) pingsize = tmp_dword;
	}
	
	if(config_key.QueryDWORDValue(L"MaxLRU", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = maxLRU;
		config_key.SetDWORDValue(L"MaxLRU", tmp_dword);
	} else {
		if(!hasMaxLRUFromCmdLine) maxLRU = tmp_dword;
	}
	
	if(config_key.QueryDWORDValue(L"UseDNS", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = useDNS ? 1 : 0;
		config_key.SetDWORDValue(L"UseDNS", tmp_dword);
	} else {
		if(!hasUseDNSFromCmdLine) useDNS = (BOOL)tmp_dword;
	}

	if(config_key.QueryDWORDValue(L"Interval", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = static_cast<DWORD>(interval * 1000);
		config_key.SetDWORDValue(L"Interval", tmp_dword);
	} else {
		if(!hasIntervalFromCmdLine) interval = (float)tmp_dword / 1000.0;
	}
	CRegKey lru_key;
	if(lru_key.Create(versionKey,
		L"LRU",
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS) != ERROR_SUCCESS)
		return FALSE;
	if(lru_key.QueryDWORDValue(NrLRU_REG_KEY, tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = nrLRU;
		lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);
	} else {
		wchar_t key_name[20];
		wchar_t str_host[NI_MAXHOST];
		nrLRU = tmp_dword;
		constexpr auto key_name_size = std::size(key_name) - 1;
		for(int i=0;i<maxLRU;i++) {
			auto result = std::format_to_n(key_name, key_name_size, reg_host_fmt, i+1);
			*result.out = '\0';
			auto value_size = static_cast<DWORD>(std::size(str_host));
			if (lru_key.QueryStringValue(key_name, str_host, &value_size) == ERROR_SUCCESS) {
				str_host[value_size]=L'\0';
				m_comboHost.AddString((CString)str_host);
			}
		}
	}
	m_comboHost.AddString(CString((LPCTSTR)IDS_STRING_CLEAR_HISTORY));
	return TRUE;
}


//*****************************************************************************
// WinMTRDialog::OnSizing
//
// 
//*****************************************************************************
void WinMTRDialog::OnSizing(UINT fwSide, LPRECT pRect) 
{
	CDialog::OnSizing(fwSide, pRect);

	int iWidth = (pRect->right)-(pRect->left);
	int iHeight = (pRect->bottom)-(pRect->top);

	if (iWidth < 600)
		pRect->right = pRect->left + 600;
	if (iHeight <250)
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
		m_staticS.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width()-lb.TopLeft().x- scaledXOffset, lb.Height() , SWP_NOMOVE | SWP_NOZORDER);
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
		m_buttonExit.SetWindowPos(nullptr, r.Width() - lb.Width()- scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
	}
	
	if (::IsWindow(m_buttonExpH.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_buttonExpH.m_hWnd);
		m_buttonExpH.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(21, dpi, 96);
		m_buttonExpH.SetWindowPos(nullptr, r.Width() - lb.Width()-scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
	}
	if (::IsWindow(m_buttonExpT.m_hWnd)) {
		const auto dpi = GetDpiForWindow(m_buttonExpT.m_hWnd);
		m_buttonExpT.GetWindowRect(&lb);
		ScreenToClient(&lb);
		const auto scaledXOffset = MulDiv(103, dpi, 96);
		m_buttonExpT.SetWindowPos(nullptr, r.Width() - lb.Width()- scaledXOffset, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
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

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
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
	return (HCURSOR) m_hIcon;
}


//*****************************************************************************
// WinMTRDialog::OnDblclkList
//
//*****************************************************************************
void WinMTRDialog::OnDblclkList([[maybe_unused]] NMHDR* pNMHDR, LRESULT* pResult)
{
	using namespace std::string_view_literals;
	*pResult = 0;

	if(state == STATES::TRACING) {
		
		POSITION pos = m_listMTR.GetFirstSelectedItemPosition();
		if(pos!=nullptr) {
			int nItem = m_listMTR.GetNextSelectedItem(pos);
			WinMTRProperties wmtrprop;
			
			if(const auto lstate = wmtrnet->getStateAt(nItem); !isValidAddress(lstate.addr)) {
				wmtrprop.host.clear();
				wmtrprop.ip.clear();
				wmtrprop.comment = lstate.getName();

				wmtrprop.pck_loss = wmtrprop.pck_sent = wmtrprop.pck_recv = 0;

				wmtrprop.ping_avrg = wmtrprop.ping_last = 0.0;
				wmtrprop.ping_best = wmtrprop.ping_worst = 0.0;
			} else {
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
// WinMTRDialog::OnRestart
//
// 
//*****************************************************************************
void WinMTRDialog::OnRestart() noexcept
{
	// If clear history is selected, just clear the registry and listbox and return
	if(m_comboHost.GetCurSel() == m_comboHost.GetCount() - 1) {
		ClearHistory();
		return;
	}

	CString sHost;
	if(state == STATES::IDLE) {
		m_comboHost.GetWindowTextW(sHost);
		sHost.TrimLeft();
		sHost.TrimLeft();
      
		if(sHost.IsEmpty()) {
			AfxMessageBox(L"No host specified!");
			m_comboHost.SetFocus();
			return ;
		}
		m_listMTR.DeleteAllItems();
	}

	if(state == STATES::IDLE) {

		if(InitMTRNet()) {
			if(m_comboHost.FindString(-1, sHost) == CB_ERR) {
				m_comboHost.InsertString(m_comboHost.GetCount() - 1,sHost);

				wchar_t key_name[20];
				CRegKey lru_key;
				lru_key.Open(HKEY_CURRENT_USER, lru_key_name, KEY_ALL_ACCESS);

				if(nrLRU >= maxLRU)
					nrLRU = 0;
				
				nrLRU++;
				auto result = std::format_to_n(key_name, std::size(key_name) - 1, reg_host_fmt, nrLRU);
				*result.out = '\0';
				lru_key.SetStringValue(key_name, static_cast<LPCWSTR>(sHost));
				auto tmp_dword =  static_cast<DWORD>(nrLRU);
				lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);
			}
			Transit(STATES::TRACING);
		}
	} else {
		Transit(STATES::STOPPING);
	}
}


//*****************************************************************************
// WinMTRDialog::OnOptions
//
// 
//*****************************************************************************
void WinMTRDialog::OnOptions() 
{
	WinMTROptions optDlg;

	optDlg.SetPingSize(pingsize);
	optDlg.SetInterval(interval);
	optDlg.SetMaxLRU(maxLRU);
	optDlg.SetUseDNS(useDNS);
	optDlg.SetUseIPv4(useIPv4);
	optDlg.SetUseIPv6(useIPv6);

	if(IDOK == optDlg.DoModal()) {

		pingsize = optDlg.GetPingSize();
		interval = optDlg.GetInterval();
		maxLRU = optDlg.GetMaxLRU();
		useDNS = optDlg.GetUseDNS();
		useIPv4 = optDlg.GetUseIPv4();
		useIPv6 = optDlg.GetUseIPv6();

		/*HKEY hKey;*/
		DWORD tmp_dword;
		wchar_t key_name[20];
		{
			CRegKey config_key;
			config_key.Open(HKEY_CURRENT_USER, config_key_name, KEY_ALL_ACCESS);
			tmp_dword = pingsize;
			config_key.SetDWORDValue(L"PingSize", tmp_dword);
			tmp_dword = maxLRU;
			config_key.SetDWORDValue(L"MaxLRU", tmp_dword);
			tmp_dword = useDNS ? 1 : 0;
			config_key.SetDWORDValue(L"UseDNS", tmp_dword);
			tmp_dword = static_cast<DWORD>(interval * 1000);
			config_key.SetDWORDValue(L"Interval", tmp_dword);
		}
		if(maxLRU<nrLRU) {
			CRegKey lru_key;
			lru_key.Open(HKEY_CURRENT_USER, lru_key_name, KEY_ALL_ACCESS);
			constexpr auto key_name_size = std::size(key_name) - 1;
			for(int i = maxLRU; i<=nrLRU; i++) {
				auto result = std::format_to_n(key_name, key_name_size, reg_host_fmt, i);
				*result.out = '\0';
				lru_key.DeleteValue(key_name);
			}
			nrLRU = maxLRU;
			tmp_dword = nrLRU;
			lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);
		}
	}
}

namespace {
	[[nodiscard]]
	std::wstring makeTextOutput(const WinMTRNet& wmtrnet) {
		using namespace std::literals;
		std::wostringstream out_buf;
		out_buf << L"|-------------------------------------------------------------------------------------------|\r\n" \
			L"|                                      WinMTR statistics                                    |\r\n" \
			L"|                       Host              -   %%  | Sent | Recv | Best | Avrg | Wrst | Last |\r\n" \
			L"|-------------------------------------------------|------|------|------|------|------|------|\r\n"sv;
		std::ostream_iterator<wchar_t, wchar_t> out(out_buf);
		CString noResponse;
		noResponse.LoadStringW(IDS_STRING_NO_RESPONSE_FROM_HOST);
		
		for (const auto curr_state = wmtrnet.getCurrentState(); const auto& hop : curr_state) {
			auto name = hop.getName();
			if (name.empty()) {
				name = noResponse;
			}
			std::format_to(out, L"| {:40} - {:4} | {:4} | {:4} | {:4} | {:4} | {:4} | {:4} |\r\n"sv,
				name, hop.getPercent(),
				hop.xmit, hop.returned, hop.best,
				hop.getAvg(), hop.worst, hop.last);
		}

		out_buf << L"|_________________________________________________|______|______|______|______|______|______|\r\n"sv;

		CString cs_tmp;
		(void)cs_tmp.LoadStringW(IDS_STRING_SB_NAME);
		out_buf << L"   "sv << cs_tmp.GetString();
		return out_buf.str();
	}

	std::wostream& makeHTMLOutput(WinMTRNet& wmtrnet, std::wostream& out) {
		using namespace std::literals;
		out << L"<table>" \
			L"<thead><tr><th>Host</th><th>%%</th><th>Sent</th><th>Recv</th><th>Best</th><th>Avrg</th><th>Wrst</th><th>Last</th></tr></thead><tbody>"sv;
		std::ostream_iterator<wchar_t, wchar_t> outitr(out);

		CString noResponse;
		noResponse.LoadStringW(IDS_STRING_NO_RESPONSE_FROM_HOST);

		for (const auto curr_state = wmtrnet.getCurrentState(); const auto& hop : curr_state) {
			auto name = hop.getName();
			if (name.empty()) {
				name = noResponse;
			}
			std::format_to(outitr
				, L"<tr><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td></tr>"sv
				, name
				, hop.getPercent()
				, hop.xmit
				, hop.returned
				, hop.best
				, hop.getAvg()
				, hop.worst
				, hop.last
			);
		}

		out<< L"</tbody></table>"sv;
		return out;
	}
}


//*****************************************************************************
// WinMTRDialog::OnCTTC
//
// 
//*****************************************************************************
void WinMTRDialog::OnCTTC() noexcept
{	
	using namespace winrt::Windows::ApplicationModel::DataTransfer;
	const auto f_buf = makeTextOutput(*wmtrnet);
	auto dataPackage = DataPackage();
	dataPackage.SetText(f_buf);

	Clipboard::SetContentWithOptions(dataPackage, nullptr);
}


//*****************************************************************************
// WinMTRDialog::OnCHTC
//
// 
//*****************************************************************************
void WinMTRDialog::OnCHTC() noexcept
{	
	using namespace winrt::Windows::ApplicationModel::DataTransfer;
	std::wostringstream out;
	makeHTMLOutput(*wmtrnet, out);
	const auto f_buf = out.str();
	const auto htmlFormat = HtmlFormatHelper::CreateHtmlFormat(f_buf);
	auto dataPackage = DataPackage();
	dataPackage.SetHtmlFormat(htmlFormat);
	Clipboard::SetContentWithOptions(dataPackage, nullptr);
}


//*****************************************************************************
// WinMTRDialog::OnEXPT
//
// 
//*****************************************************************************
void WinMTRDialog::OnEXPT() noexcept
{	
	const TCHAR BASED_CODE szFilter[] = _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");

	CFileDialog dlg(FALSE,
                   _T("TXT"),
                   nullptr,
                   OFN_HIDEREADONLY,
                   szFilter,
                   this);
	if(dlg.DoModal() == IDOK) {
		const auto f_buf = makeTextOutput(*wmtrnet);

		if(std::wfstream fp(dlg.GetPathName(), std::ios::binary | std::ios::out | std::ios::trunc); fp) {
			fp << f_buf << std::endl;
		}
	}
}


//*****************************************************************************
// WinMTRDialog::OnEXPH
//
// 
//*****************************************************************************
void WinMTRDialog::OnEXPH() noexcept
{
   const TCHAR szFilter[] = _T("HTML Files (*.htm, *.html)|*.htm;*.html|All Files (*.*)|*.*||");

   CFileDialog dlg(FALSE,
                   _T("HTML"),
                   nullptr,
                   OFN_HIDEREADONLY,
                   szFilter,
                   this);

	if(dlg.DoModal() == IDOK) {
		if (std::wfstream fp(dlg.GetPathName(), std::ios::binary | std::ios::out | std::ios::trunc); fp) {
			fp << L"<!DOCTYPE html><html><head><meta charset=\"utf-8\"/><title>WinMTR Statistics</title><style>" \
				L"td{padding:0.2rem 1rem;border:1px solid #000;}"
				L"table{border:1px solid #000;border-collapse:collapse;}" \
				L"tbody tr:nth-child(even){background:#ccc;}</style></head><body>" \
				L"<h1>WinMTR statistics</h1>"sv;
			makeHTMLOutput(*wmtrnet, fp) << L"</body></html>"sv << std::endl;
		}
	}
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
		
		auto result = std::format_to_n(nr_crt, std::size(nr_crt) - 1, WinMTRUtils::int_number_format, i+1);
		*result.out = '\0';
		if(m_listMTR.GetItemCount() <= i )
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


//*****************************************************************************
// WinMTRDialog::InitMTRNet
//
// 
//*****************************************************************************
bool WinMTRDialog::InitMTRNet() noexcept
{
	CString sHost;
	m_comboHost.GetWindowTextW(sHost);

	if (sHost.IsEmpty()) [[unlikely]] { // Technically never because this is caught in the calling function
		sHost = L"localhost";
	}

	SOCKADDR_STORAGE addrstore{};
	for (auto af : { AF_INET, AF_INET6 }) {
		INT addrSize = sizeof(addrstore);
		if (auto res = WSAStringToAddressW(
			sHost.GetBuffer()
			, af
			, nullptr
			, reinterpret_cast<LPSOCKADDR>(&addrstore)
			, &addrSize);
			!res) {
			return true;
		}
	}

	const auto buf = std::format(L"Resolving host {}..."sv, sHost.GetString());
	statusBar.SetPaneText(0, buf.c_str());
	std::unique_ptr<ADDRINFOEXW, addrinfo_deleter> holder;
	ADDRINFOEXW hint = { .ai_family = AF_UNSPEC };
	if (const auto result = GetAddrInfoExW(
		sHost
		, nullptr
		, NS_ALL
		, nullptr
		, &hint
		, std::out_ptr(holder)
		, nullptr
		, nullptr
		, nullptr
		, nullptr); result != ERROR_SUCCESS) {
		statusBar.SetPaneText(0, CString((LPCTSTR)IDS_STRING_SB_NAME));
		AfxMessageBox(IDS_STRING_UNABLE_TO_RESOLVE_HOSTNAME);
		return false;
	}

	return true;
}

void WinMTRDialog::OnCbnSelchangeComboHost()
{
}

void WinMTRDialog::ClearHistory()
{
	DWORD tmp_dword;
	wchar_t key_name[20] = {};
	CRegKey lru_key;
	lru_key.Open(HKEY_CURRENT_USER, LR"(Software\WinMTR\LRU)", KEY_ALL_ACCESS);
	constexpr auto key_name_size = std::size(key_name) - 1;
	for(int i = 0; i<=nrLRU; i++) {
		auto result = std::format_to_n(key_name, key_name_size, reg_host_fmt, i);
		*result.out = '\0';
		lru_key.DeleteValue(key_name);
	}
	nrLRU = 0;
	tmp_dword = nrLRU;
	lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);

	m_comboHost.Clear();
	m_comboHost.ResetContent();
	m_comboHost.AddString(CString((LPCSTR)IDS_STRING_CLEAR_HISTORY));
}

winrt::Windows::Foundation::IAsyncAction WinMTRDialog::pingThread(std::stop_token stop_token, std::wstring sHost)
{
	if (tracing.exchange(true)) {
		throw new std::runtime_error("Tracing started twice!");
	}
	struct tracexit {
		WinMTRDialog* dialog;
		~tracexit() noexcept {
			dialog->tracing.store(false, std::memory_order_release);
		}
	}tracexit{this};

	SOCKADDR_STORAGE addrstore = {};
	
	for (auto af : { AF_INET, AF_INET6 }) {
		INT addrSize = sizeof(addrstore);
		if (auto res = WSAStringToAddressW(
			sHost.data()
			, af
			, nullptr
			, reinterpret_cast<LPSOCKADDR>(&addrstore)
			, &addrSize);
			!res) {
			co_await this->wmtrnet->DoTrace(stop_token, *reinterpret_cast<LPSOCKADDR>(&addrstore));
			co_return;
		}
	}
	
	int  hintFamily = AF_UNSPEC; //both
	if (!this->useIPv4) {
		hintFamily = AF_INET6;
	}
	else if (!this->useIPv6) {
		hintFamily = AF_INET;
	}
	timeval timeout{ .tv_sec = 30 };
	auto result = co_await GetAddrInfoAsync(sHost, &timeout, hintFamily);
	if (!result || result->empty()) {
		AfxMessageBox(IDS_STRING_UNABLE_TO_RESOLVE_HOSTNAME);
		co_return;
	}
	addrstore = result->front();
	co_await this->wmtrnet->DoTrace(stop_token, *reinterpret_cast<LPSOCKADDR>(&addrstore));
}

winrt::fire_and_forget WinMTRDialog::stopTrace()
{
	// grab the thread under a mutex so we don't mess this up and cause a data race
	decltype(trace_lacky) temp;
	{
		std::unique_lock trace_lock{ tracer_mutex };
		std::swap(temp, trace_lacky);
	}
	// don't bother trying call something not there
	if (!temp) {
		co_return;
	}
	co_await winrt::resume_background();
	temp.reset(); //trigger the stop token
	co_return;
}

void WinMTRDialog::OnCbnSelendokComboHost()
{
}


void WinMTRDialog::OnCbnCloseupComboHost()
{
	if(m_comboHost.GetCurSel() == m_comboHost.GetCount() - 1) {
		ClearHistory();
	}
}

void WinMTRDialog::Transit(STATES new_state)
{
	switch(new_state) {
	case STATES::IDLE:
			switch (state) {
			case STATES::STOPPING:
					transition = STATE_TRANSITIONS::STOPPING_TO_IDLE;
				break;
			case STATES::IDLE:
					transition = STATE_TRANSITIONS::IDLE_TO_IDLE;
				break;
				default:
					TRACE_MSG( L"Received state IDLE after " << static_cast<int>(state));
					return;
			}
			state = STATES::IDLE;
		break;
	case STATES::TRACING:
			switch (state) {
				case STATES::IDLE:
					transition = STATE_TRANSITIONS::IDLE_TO_TRACING;
				break;
				case STATES::TRACING:
					transition = STATE_TRANSITIONS::TRACING_TO_TRACING;
				break;
				default:
					TRACE_MSG(L"Received state TRACING after " << static_cast<int>(state));
					return;
			}
			state = STATES::TRACING;
		break;
		case STATES::STOPPING:
			switch (state) {
				case STATES::STOPPING:
					transition = STATE_TRANSITIONS::STOPPING_TO_STOPPING;
				break;
				case STATES::TRACING:
					transition = STATE_TRANSITIONS::TRACING_TO_STOPPING;
				break;
				default:
					TRACE_MSG(L"Received state STOPPING after " << static_cast<int>(state));
					return;
			}
			state = STATES::STOPPING;
		break;
		case STATES::EXIT:
			switch (state) {
				case STATES::IDLE:
					transition = STATE_TRANSITIONS::IDLE_TO_EXIT;
				break;
				case STATES::STOPPING:
					transition = STATE_TRANSITIONS::STOPPING_TO_EXIT;
				break;
				case STATES::TRACING:
					transition = STATE_TRANSITIONS::TRACING_TO_EXIT;
				break;
				case STATES::EXIT:
				break;
				default:
					TRACE_MSG(L"Received state EXIT after " << static_cast<int>(state));
					return;
			}
			state = STATES::EXIT;
		break;
		default:
			TRACE_MSG(L"Received state " << static_cast<std::underlying_type_t<STATES>>(state));
	}

	// modify controls according to new state
	switch(transition) {
		case STATE_TRANSITIONS::IDLE_TO_TRACING:
			{
				m_buttonStart.EnableWindow(FALSE);
				CString newText;
				newText.LoadStringW(IDS_STRING_STOP);
				m_buttonStart.SetWindowText(newText);
				m_comboHost.EnableWindow(FALSE);
				m_buttonOptions.EnableWindow(FALSE);
				newText.LoadStringW(IDS_STRING_DBL_CLICK_MORE_INFO);
				statusBar.SetPaneText(0, newText);
				// using a different thread to create an MTA so we don't have explosion issues with the
				// thread pool
				CString sHost;
				this->m_comboHost.GetWindowTextW(sHost);

				if (sHost.IsEmpty()) [[unlikely]] { // Technically never because this is caught in the calling function
					sHost = L"localhost";
				}
				std::unique_lock trace_lock{ tracer_mutex };
				// create the jthread and stop token all in one go
				trace_lacky.emplace([this](std::stop_token stop_token, auto sHost) noexcept {
					winrt::init_apartment(winrt::apartment_type::multi_threaded);
					try {
						auto tracer_local = this->pingThread(stop_token, sHost);
						{
							std::unique_lock lock(this->tracer_mutex);
						}
						// keep the thread alive
						tracer_local.get();
					}
					catch (winrt::hresult_canceled const&) {
						// don't care this happens
					}
					catch (winrt::hresult_illegal_method_call const&){
						// don't care this happens
					}
					}, std::wstring(sHost));
			}
			m_buttonStart.EnableWindow(TRUE);
		break;
		case STATE_TRANSITIONS::IDLE_TO_IDLE:
			// nothing to be done
		break;
		case STATE_TRANSITIONS::STOPPING_TO_IDLE:
		{
			CString newText;
			newText.LoadStringW(IDS_STRING_START);
			m_buttonStart.EnableWindow(TRUE);
			statusBar.SetPaneText(0, CString((LPCSTR)IDS_STRING_SB_NAME));
			m_buttonStart.SetWindowText(newText);
			m_comboHost.EnableWindow(TRUE);
			m_buttonOptions.EnableWindow(TRUE);
			m_comboHost.SetFocus();
		}
			
		break;
		case STATE_TRANSITIONS::STOPPING_TO_STOPPING:
			DisplayRedraw();
		break;
		case STATE_TRANSITIONS::TRACING_TO_TRACING:
			DisplayRedraw();
		break;
		case STATE_TRANSITIONS::TRACING_TO_STOPPING:
		{
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
			this->stopTrace();
			CString newText;
			newText.LoadStringW(IDS_STRING_WAITING_STOP_TRACE);
			statusBar.SetPaneText(0, newText);
			DisplayRedraw();
		}
		break;
		case STATE_TRANSITIONS::IDLE_TO_EXIT:
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
		break;
		case STATE_TRANSITIONS::TRACING_TO_EXIT:
		{
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
			this->stopTrace();
			CString newText;
			newText.LoadStringW(IDS_STRING_WAITING_STOP_TRACE);
			statusBar.SetPaneText(0, newText);
		}
		break;
		case STATE_TRANSITIONS::STOPPING_TO_EXIT:
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
		break;
		default:
			TRACE_MSG("Unknown transition " << static_cast<std::underlying_type_t<STATE_TRANSITIONS>>(transition));
	}
}


void WinMTRDialog::OnTimer(UINT_PTR nIDEvent) noexcept
{
	static unsigned int call_count = 0;
	call_count += 1;
	//std::unique_lock lock(traceThreadMutex, std::try_to_lock);
	const bool is_tracing = tracing.load(std::memory_order_acquire);
	if(state == STATES::EXIT && !is_tracing) {
		OnOK();
	}

	if(!is_tracing) {
		Transit(STATES::IDLE);
	} else if( (call_count % 10 == 0)) {
		if( state == STATES::TRACING) Transit(STATES::TRACING);
		else if( state == STATES::STOPPING) Transit(STATES::STOPPING);
	}

	CDialog::OnTimer(nIDEvent);
}


void WinMTRDialog::OnClose()
{
	Transit(STATES::EXIT);
}


void WinMTRDialog::OnBnClickedCancel()
{
	Transit(STATES::EXIT);
}
