//*****************************************************************************
// FILE:            WinMTRDialog.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************

#ifndef WINMTRDIALOG_H_
#define WINMTRDIALOG_H_

#define WINMTR_DIALOG_TIMER 100

#include "WinMTRStatusBar.h"
#include "WinMTRNet.h"
#include <string>
#include <memory>
#include <mutex>
#include "resource.h"


//*****************************************************************************
// CLASS:  WinMTRDialog
//
//
//*****************************************************************************

class WinMTRDialog final: public CDialog
{
public:
	WinMTRDialog(CWnd* pParent = NULL);

	enum { IDD = IDD_WINMTR_DIALOG };
	enum class options_source : bool {
		none,
		cmd_line
	};

	afx_msg BOOL InitRegistry();

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

	
	
	int InitMTRNet();

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
	//std::recursive_mutex			traceThreadMutex;
	std::shared_ptr<WinMTRNet>			wmtrnet;
	std::optional<winrt::Windows::Foundation::IAsyncAction> tracer;
	std::optional<winrt::apartment_context> context;
	HICON m_hIcon;
	double				interval;
	STATES				state;
	STATE_TRANSITIONS	transition;
	int					pingsize;
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
	std::atomic_bool	tracing = false;

	void ClearHistory();
	winrt::Windows::Foundation::IAsyncAction pingThread();
	winrt::fire_and_forget stopTrace();
public:

	void SetHostName(std::wstring host);
	void SetInterval(float i, options_source fromCmdLine = options_source::none) noexcept;
	void SetPingSize(int ps, options_source fromCmdLine = options_source::none) noexcept;
	void SetMaxLRU(int mlru, options_source fromCmdLine = options_source::none) noexcept;
	void SetUseDNS(bool udns, options_source fromCmdLine = options_source::none) noexcept;

	inline auto getInterval() const noexcept { return interval; }
	inline auto getPingSize() const noexcept { return pingsize; }
	inline auto getUseDNS() const noexcept { return useDNS; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT, int, int);
	afx_msg void OnSizing(UINT, LPRECT); 
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRestart();
	afx_msg void OnOptions();
	virtual void OnCancel();

	afx_msg void OnCTTC();
	afx_msg void OnCHTC();
	afx_msg void OnEXPT();
	afx_msg void OnEXPH();

	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeComboHost();
	afx_msg void OnCbnSelendokComboHost();
	afx_msg void OnCbnCloseupComboHost();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnBnClickedCancel();
};

#endif // ifndef WINMTRDIALOG_H_
