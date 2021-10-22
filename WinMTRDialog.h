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
#include "IWinMTROptionsProvider.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <optional>
#include <atomic>
#include <winrt/Windows.Foundation.h>
#include "resource.h"

class WinMTRNet;
//*****************************************************************************
// CLASS:  WinMTRDialog
//
//
//*****************************************************************************

class WinMTRDialog final: public CDialog, public IWinMTROptionsProvider
{
public:
	WinMTRDialog(CWnd* pParent = nullptr);

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

	
	
	bool InitMTRNet();

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
	struct tracer_lacky {
		winrt::Windows::Foundation::IAsyncAction tracer;
		winrt::apartment_context context;
	};
	std::mutex tracer_mutex;
	std::optional<tracer_lacky> trace_lacky;
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
	winrt::Windows::Foundation::IAsyncAction pingThread(std::wstring shost);
	winrt::fire_and_forget stopTrace();
public:

	void SetHostName(std::wstring host);
	void SetInterval(float i, options_source fromCmdLine = options_source::none) noexcept;
	void SetPingSize(int ps, options_source fromCmdLine = options_source::none) noexcept;
	void SetMaxLRU(int mlru, options_source fromCmdLine = options_source::none) noexcept;
	void SetUseDNS(bool udns, options_source fromCmdLine = options_source::none) noexcept;

	inline double getInterval() const noexcept { return interval; }
	inline int getPingSize() const noexcept { return pingsize; }
	inline bool getUseDNS() const noexcept { return useDNS; }

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
