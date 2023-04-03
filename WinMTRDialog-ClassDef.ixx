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

export module WinMTR.Dialog:ClassDef;

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
	std::atomic<double>				interval;
	STATES				state;
	STATE_TRANSITIONS	transition;
	std::atomic_uint	pingsize;
	int					maxLRU;
	int					nrLRU = 0;
	bool				m_autostart = false;
	bool				hasPingsizeFromCmdLine = false;
	bool				hasMaxLRUFromCmdLine = false;
	bool				hasIntervalFromCmdLine = false;
	std::atomic_bool				useDNS;
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