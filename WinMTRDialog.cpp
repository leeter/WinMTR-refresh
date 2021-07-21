//*****************************************************************************
// FILE:            WinMTRDialog.cpp
//
//
//*****************************************************************************
#include "WinMTRGlobal.h"

#include "WinMTRDialog.h"
#include "WinMTROptions.h"
#include "WinMTRProperties.h"
#include "WinMTRNet.h"

#include "afxlinkctrl.h"


using namespace std::string_view_literals;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static	 char THIS_FILE[] = __FILE__;
#endif



namespace {
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
WinMTRDialog::WinMTRDialog(CWnd* pParent)
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

	#ifndef  _WIN64
	const wchar_t caption[] = {L"WinMTR v0.92 32 bit by Appnor MSP - www.winmtr.net"};
	#else
	const wchar_t caption[] = {L"WinMTR v0.92 64 bit by Appnor MSP - www.winmtr.net"};
	#endif

	SetTimer(1, WINMTR_DIALOG_TIMER, nullptr);
	SetWindowTextW(caption);

	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);
	
	if(!statusBar.Create( this ))
		AfxMessageBox(L"Error creating status bar");
	statusBar.GetStatusBarCtrl().SetMinHeight(23);
		
	UINT sbi[1];
	sbi[0] = IDS_STRING_SB_NAME;	
	statusBar.SetIndicators(sbi);
	statusBar.SetPaneInfo(0, statusBar.GetItemID(0),SBPS_STRETCH, 0 );
	{ // Add appnor URL
		std::unique_ptr<CMFCLinkCtrl> m_pWndButton = std::make_unique<CMFCLinkCtrl>();
		if (!m_pWndButton->Create(_T("www.appnor.com"), WS_CHILD|WS_VISIBLE|WS_TABSTOP, CRect(0,0,0,0), &statusBar, 1234)) {
			TRACE(_T("Failed to create button control.\n"));
			return FALSE;
		}

		m_pWndButton->SetURL(L"http://www.appnor.com/?utm_source=winmtr&utm_medium=desktop&utm_campaign=software");
			
		if(!statusBar.AddPane(1234,1)) {
			AfxMessageBox(_T("Pane index out of range\nor pane with same ID already exists in the status bar"), MB_ICONERROR);
			return FALSE;
		}
			
		statusBar.SetPaneWidth(statusBar.CommandToIndex(1234), 100);
		statusBar.AddPaneControl(m_pWndButton.release(), 1234, true);
	}

	for(int i = 0; i< MTR_NR_COLS; i++)
		m_listMTR.InsertColumn(i, MTR_COLS[i], LVCFMT_LEFT, MTR_COL_LENGTH[i] , -1);
   
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
	CPoint ptOffset(rcClientNow.left - rcClientStart.left,
					rcClientNow.top - rcClientStart.top);

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
// WinMTRDialog::InitRegistry
//
// 
//*****************************************************************************
BOOL WinMTRDialog::InitRegistry()
{
	CRegKey versionKey;
	if (versionKey.Create(HKEY_CURRENT_USER,
		LR"(Software\WinMTR)",
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS) != ERROR_SUCCESS){
		return FALSE;

	}
	versionKey.SetStringValue(L"Version", WINMTR_VERSION);
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
	if(lru_key.QueryDWORDValue(L"NrLRU", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = nrLRU;
		lru_key.SetDWORDValue(L"NrLRU", tmp_dword);
	} else {
		wchar_t key_name[20];
		wchar_t str_host[NI_MAXHOST];
		nrLRU = tmp_dword;
		for(int i=0;i<maxLRU;i++) {
			std::swprintf(key_name, std::size(key_name), L"Host%d", i+1);
			auto value_size = gsl::narrow_cast<DWORD>(std::size(str_host));
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
		m_staticS.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_staticS.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width()-lb.TopLeft().x-10, lb.Height() , SWP_NOMOVE | SWP_NOZORDER);
	}

	if (::IsWindow(m_staticJ.m_hWnd)) {
		m_staticJ.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_staticJ.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width() - 21, lb.Height(), SWP_NOMOVE | SWP_NOZORDER);
	}

	if (::IsWindow(m_buttonExit.m_hWnd)) {
		m_buttonExit.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_buttonExit.SetWindowPos(nullptr, r.Width() - lb.Width()-21, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
	}
	
	if (::IsWindow(m_buttonExpH.m_hWnd)) {
		m_buttonExpH.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_buttonExpH.SetWindowPos(nullptr, r.Width() - lb.Width()-21, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
	}
	if (::IsWindow(m_buttonExpT.m_hWnd)) {
		m_buttonExpT.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_buttonExpT.SetWindowPos(nullptr, r.Width() - lb.Width()- 103, lb.TopLeft().y, lb.Width(), lb.Height() , SWP_NOSIZE | SWP_NOZORDER);
	}

	if (::IsWindow(m_listMTR.m_hWnd)) {
		m_listMTR.GetWindowRect(&lb);
		ScreenToClient(&lb);
		m_listMTR.SetWindowPos(nullptr, lb.TopLeft().x, lb.TopLeft().y, r.Width() - 21, r.Height() - lb.top - 25, SWP_NOMOVE | SWP_NOZORDER);
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
			
			if(auto addr = wmtrnet->GetAddr(nItem); !isValidAddress(addr)) {
				wmtrprop.host.clear();
				wmtrprop.ip.clear();
				wmtrprop.comment = wmtrnet->GetName(nItem);

				wmtrprop.pck_loss = wmtrprop.pck_sent = wmtrprop.pck_recv = 0;

				wmtrprop.ping_avrg = wmtrprop.ping_last = 0.0;
				wmtrprop.ping_best = wmtrprop.ping_worst = 0.0;
			} else {
				wmtrprop.host = wmtrnet->GetName(nItem);
				wmtrprop.ip.resize(40);
				auto addrlen = getAddressSize(addr);
				DWORD addrstrsize = static_cast<DWORD>(wmtrprop.ip.size());
				auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), static_cast<DWORD>(addrlen), nullptr, wmtrprop.ip.data(), &addrstrsize);
				if (!result) {
					wmtrprop.ip.resize(addrstrsize - 1);
				}
				
				wmtrprop.comment = L"Host alive."sv;

				wmtrprop.ping_avrg = static_cast<float>(wmtrnet->GetAvg(nItem)); 
				wmtrprop.ping_last = static_cast<float>(wmtrnet->GetLast(nItem)); 
				wmtrprop.ping_best = static_cast<float>(wmtrnet->GetBest(nItem));
				wmtrprop.ping_worst = static_cast<float>(wmtrnet->GetWorst(nItem)); 

				wmtrprop.pck_loss = wmtrnet->GetPercent(nItem);
				wmtrprop.pck_recv = wmtrnet->GetReturned(nItem);
				wmtrprop.pck_sent = wmtrnet->GetXmit(nItem);
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
void WinMTRDialog::SetPingSize(int ps, options_source fromCmdLine) noexcept
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
void WinMTRDialog::OnRestart() 
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
				std::swprintf(key_name, std::size(key_name), L"Host%d", nrLRU);
				const auto sHostLen = sHost.GetAllocLength() * sizeof(wchar_t) + sizeof(wchar_t);
				lru_key.SetStringValue(key_name, static_cast<LPCWSTR>(sHost));
				auto tmp_dword =  static_cast<DWORD>(nrLRU);
				lru_key.SetDWORDValue(L"NrLRU", tmp_dword);
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

			for(int i = maxLRU; i<=nrLRU; i++) {
				std::swprintf(key_name, std::size(key_name), L"Host%d", i);
				lru_key.DeleteValue(key_name);
			}
			nrLRU = maxLRU;
			tmp_dword = nrLRU;
			lru_key.SetDWORDValue(L"NrLRU", tmp_dword);
		}
	}
}

namespace {
	[[nodiscard]]
	std::wstring makeTextOutput(WinMTRNet& wmtrnet) {
		using namespace std::literals;
		std::vector<wchar_t> t_buf(1000);
		const int nh = wmtrnet.GetMax();
		std::wstring f_buf;
		f_buf.reserve(255 * 100);
		f_buf += L"|-------------------------------------------------------------------------------------------|\r\n" \
			L"|                                      WinMTR statistics                                    |\r\n" \
			L"|                       Host              -   %%  | Sent | Recv | Best | Avrg | Wrst | Last |\r\n" \
			L"|-------------------------------------------------|------|------|------|------|------|------|\r\n"sv;

		for (int i = 0; i < nh; i++) {
			auto name = wmtrnet.GetName(i);
			if (name.empty()) {
				name = L"No response from host"s;
			}

			std::swprintf(t_buf.data(), std::size(t_buf), L"|%40ws - %4d | %4d | %4d | %4d | %4d | %4d | %4d |\r\n",
				name.c_str(), wmtrnet.GetPercent(i),
				wmtrnet.GetXmit(i), wmtrnet.GetReturned(i), wmtrnet.GetBest(i),
				wmtrnet.GetAvg(i), wmtrnet.GetWorst(i), wmtrnet.GetLast(i));
			f_buf += t_buf.data();
		}

		f_buf += L"|________________________________________________|______|______|______|______|______|______|\r\n"sv;

		CString cs_tmp((LPCTSTR)IDS_STRING_SB_NAME);
		f_buf += L"   "sv;
		f_buf += cs_tmp.GetString();
		return f_buf;
	}

	[[nodiscard]]
	auto makeHTMLOutput(WinMTRNet& wmtrnet) {
		using namespace std::literals;
		const int nh = wmtrnet.GetMax();
		std::wstring f_buf;
		f_buf.reserve(255 * 100);
		f_buf += L"<!DOCTYPE html><html><head><title>WinMTR Statistics</title></head><body>\r\n" \
			L"<h1>WinMTR statistics</h1>\r\n" \
			L"<table>\r\n" \
			L"<thead><tr><th>Host</th> <th>%%</th> <th>Sent</th> <th>Recv</th> <th>Best</th> <th>Avrg</th> <th>Wrst</th> <th>Last</th></tr></thead><tbody>\r\n"sv;
		std::vector<wchar_t> t_buf(1000);
		for (int i = 0; i < nh; i++) {
			auto name = wmtrnet.GetName(i);
			if (name.empty()) {
				name = L"No response from host"s;
			}

			std::swprintf(t_buf.data(), std::size(t_buf), L"<tr><td>%ws</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td> <td>%4d</td></tr>\r\n",
				name.c_str(), wmtrnet.GetPercent(i),
				wmtrnet.GetXmit(i), wmtrnet.GetReturned(i), wmtrnet.GetBest(i),
				wmtrnet.GetAvg(i), wmtrnet.GetWorst(i), wmtrnet.GetLast(i));
			f_buf += t_buf.data();
		}

		f_buf += L"</tbody></table></body></html>\r\n"sv;
		return f_buf;
	}
}


//*****************************************************************************
// WinMTRDialog::OnCTTC
//
// 
//*****************************************************************************
void WinMTRDialog::OnCTTC() 
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
void WinMTRDialog::OnCHTC() 
{	
	using namespace winrt::Windows::ApplicationModel::DataTransfer;

	const auto f_buf = makeHTMLOutput(*wmtrnet);
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
void WinMTRDialog::OnEXPT() 
{	
	const TCHAR BASED_CODE szFilter[] = _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");

	CFileDialog dlg(FALSE,
                   _T("TXT"),
                   nullptr,
                   OFN_HIDEREADONLY | OFN_EXPLORER,
                   szFilter,
                   this);
	if(dlg.DoModal() == IDOK) {
		const auto f_buf = makeTextOutput(*wmtrnet);

		if(std::wfstream fp(dlg.GetPathName()); fp) {
			fp << f_buf << std::endl;
		}
	}
}


//*****************************************************************************
// WinMTRDialog::OnEXPH
//
// 
//*****************************************************************************
void WinMTRDialog::OnEXPH() 
{
   const TCHAR szFilter[] = _T("HTML Files (*.htm, *.html)|*.htm;*.html|All Files (*.*)|*.*||");

   CFileDialog dlg(FALSE,
                   _T("HTML"),
                   nullptr,
                   OFN_HIDEREADONLY | OFN_EXPLORER,
                   szFilter,
                   this);

	if(dlg.DoModal() == IDOK) {
		const auto f_buf = makeHTMLOutput(*wmtrnet);

		if (std::wfstream fp(dlg.GetPathName()); fp) {
			fp << f_buf << std::endl;
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
	wchar_t buf[255], nr_crt[255];
	const int nh = wmtrnet->GetMax();
	while (m_listMTR.GetItemCount() > nh) {
		m_listMTR.DeleteItem(m_listMTR.GetItemCount() - 1); 
	}

	for(int i=0;i <nh ; i++) {

		auto name = wmtrnet->GetName(i);
		if (name.empty()) {
			name = L"No response from host"sv;
		}
		
		std::swprintf(nr_crt, std::size(nr_crt), L"%d", i+1);
		if(m_listMTR.GetItemCount() <= i )
			m_listMTR.InsertItem(i, name.c_str());
		else
			m_listMTR.SetItem(i, 0, LVIF_TEXT, name.c_str(), 0, 0, 0, 0); 
		
		m_listMTR.SetItem(i, 1, LVIF_TEXT, nr_crt, 0, 0, 0, 0); 

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetPercent(i));
		m_listMTR.SetItem(i, 2, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetXmit(i));
		m_listMTR.SetItem(i, 3, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetReturned(i));
		m_listMTR.SetItem(i, 4, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetBest(i));
		m_listMTR.SetItem(i, 5, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetAvg(i));
		m_listMTR.SetItem(i, 6, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetWorst(i));
		m_listMTR.SetItem(i, 7, LVIF_TEXT, buf, 0, 0, 0, 0);

		std::swprintf(buf, std::size(buf), L"%d", wmtrnet->GetLast(i));
		m_listMTR.SetItem(i, 8, LVIF_TEXT, buf, 0, 0, 0, 0);

   
	}

	return 0;
}


//*****************************************************************************
// WinMTRDialog::InitMTRNet
//
// 
//*****************************************************************************
int WinMTRDialog::InitMTRNet()
{
	wchar_t strtmp[255];
	const wchar_t *Hostname = strtmp;
	m_comboHost.GetWindowText(strtmp, 255);
   	
	if (Hostname == nullptr) Hostname = L"localhost";
   
	bool isIP=true;
	const wchar_t *t = Hostname;
	while(*t) {
		if(!std::iswdigit(*t) && *t!=L'.') {
			isIP = false;
			break;
		}
		t++;
	}

	if(!isIP) {
		wchar_t buf[255];
		std::swprintf(buf, std::size(buf), L"Resolving host %s...", strtmp);
		statusBar.SetPaneText(0,buf);
		PADDRINFOW out = nullptr;
		ADDRINFOW hint = {};
		hint.ai_family = AF_INET | AF_INET6;
		if (const auto result = GetAddrInfoW(Hostname, nullptr, &hint, &out); result) {
			FreeAddrInfoW(out);
			statusBar.SetPaneText(0, CString((LPCTSTR)IDS_STRING_SB_NAME) );
			AfxMessageBox(IDS_STRING_UNABLE_TO_RESOLVE_HOSTNAME);
			return 0;
		}
		FreeAddrInfoW(out);
	}

	return 1;
}

void WinMTRDialog::OnCbnSelchangeComboHost()
{
}

void WinMTRDialog::ClearHistory()
{
	DWORD tmp_dword;
	wchar_t key_name[20];
	CRegKey lru_key;
	lru_key.Open(HKEY_CURRENT_USER, LR"(Software\WinMTR\LRU)", KEY_ALL_ACCESS);

	for(int i = 0; i<=nrLRU; i++) {
		std::swprintf(key_name, std::size(key_name), L"Host%d", i);
		lru_key.DeleteValue(key_name);
	}
	nrLRU = 0;
	tmp_dword = nrLRU;
	lru_key.SetDWORDValue(L"NrLRU", tmp_dword);

	m_comboHost.Clear();
	m_comboHost.ResetContent();
	m_comboHost.AddString(CString((LPCSTR)IDS_STRING_CLEAR_HISTORY));
}

winrt::Windows::Foundation::IAsyncAction WinMTRDialog::pingThread()
{
	if (tracing.exchange(true)) {
		throw new std::runtime_error("Tracing started twice!");
	}
	auto finally = gsl::finally([this] {
		this->tracing = false;
	});

	wchar_t strtmp[255];
	const wchar_t* Hostname = strtmp;
	SOCKADDR_STORAGE addrstore = {};
	this->m_comboHost.GetWindowTextW(strtmp, gsl::narrow_cast<int>(std::size(strtmp)));

	if (Hostname == nullptr) {
		Hostname = L"localhost";
	}

	auto cancellation = co_await winrt::get_cancellation_token();

	for (auto af : { AF_INET, AF_INET6 }) {
		INT addrSize = sizeof(addrstore);
		if (auto res = WSAStringToAddressW(
			strtmp
			, af
			, nullptr
			, reinterpret_cast<LPSOCKADDR>(&addrstore)
			, &addrSize);
			!res) {
			co_await MakeCancellable(this->wmtrnet->DoTrace(*reinterpret_cast<LPSOCKADDR>(&addrstore)), cancellation);
			co_return;
		}
	}

	PADDRINFOW out = nullptr;
	ADDRINFOW hint = { .ai_family = (this->useIPv4 ? AF_INET : 0) | (this->useIPv6 ? AF_INET6 : 0) };
	assert(hint.ai_family != 0);
	
	if (const auto result = GetAddrInfoW(Hostname, nullptr, &hint, &out); result) {
		if (out) {
			FreeAddrInfoW(out);
		}
		AfxMessageBox(L"Unable to resolve address.");
		co_return;
	}
	std::memcpy(&addrstore, out->ai_addr, out->ai_addrlen);
	FreeAddrInfoW(out);
	co_await MakeCancellable(this->wmtrnet->DoTrace(*reinterpret_cast<LPSOCKADDR>(&addrstore)), cancellation);
}

winrt::fire_and_forget WinMTRDialog::stopTrace()
{
	if (this->tracer) {
		co_await *context;
		tracer->Cancel();
	}
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
			m_buttonStart.EnableWindow(FALSE);
			m_buttonStart.SetWindowText(L"Stop");
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
			statusBar.SetPaneText(0, L"Double click on host name for more information.");
			{
				// using a different thread to create an MTA so we don't have explosion issues with the
				// thread pool
				auto thread = std::thread([this]() noexcept {
					winrt::init_apartment(winrt::apartment_type::multi_threaded);
					try {
						this->context = winrt::apartment_context();
						this->tracer = this->pingThread();
						// keep the thread alive
						tracer->get();
					}
					catch (winrt::hresult_canceled const&) {
						// don't care this happens
					}
					catch (winrt::hresult_illegal_method_call const&){
						// don't care this happens
					}
					this->tracer.reset();
					this->context.reset();
					});
				thread.detach();
			}
			m_buttonStart.EnableWindow(TRUE);
		break;
		case STATE_TRANSITIONS::IDLE_TO_IDLE:
			// nothing to be done
		break;
		case STATE_TRANSITIONS::STOPPING_TO_IDLE:
			m_buttonStart.EnableWindow(TRUE);
			statusBar.SetPaneText(0, CString((LPCSTR)IDS_STRING_SB_NAME) );
			m_buttonStart.SetWindowText(L"Start");
			m_comboHost.EnableWindow(TRUE);
			m_buttonOptions.EnableWindow(TRUE);
			m_comboHost.SetFocus();
		break;
		case STATE_TRANSITIONS::STOPPING_TO_STOPPING:
			DisplayRedraw();
		break;
		case STATE_TRANSITIONS::TRACING_TO_TRACING:
			DisplayRedraw();
		break;
		case STATE_TRANSITIONS::TRACING_TO_STOPPING:
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
			this->stopTrace();
			//wmtrnet->StopTrace();
			statusBar.SetPaneText(0, L"Waiting for last packets in order to stop trace ...");
			DisplayRedraw();
		break;
		case STATE_TRANSITIONS::IDLE_TO_EXIT:
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
		break;
		case STATE_TRANSITIONS::TRACING_TO_EXIT:
			m_buttonStart.EnableWindow(FALSE);
			m_comboHost.EnableWindow(FALSE);
			m_buttonOptions.EnableWindow(FALSE);
			this->stopTrace();
			statusBar.SetPaneText(0, L"Waiting for last packets in order to stop trace ...");
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


void WinMTRDialog::OnTimer(UINT_PTR nIDEvent)
{
	static unsigned int call_count = 0;
	call_count += 1;
	//std::unique_lock lock(traceThreadMutex, std::try_to_lock);
	const bool is_tracing = tracing;
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
