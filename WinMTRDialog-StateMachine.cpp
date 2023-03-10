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
 
#include "resource.h"

module WinMTR.Dialog:StateMachine;

import :ClassDef;
import <mutex>;
import <string>;
import <winrt/Windows.Foundation.h>;

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



void WinMTRDialog::Transit(STATES new_state)
{
	switch (new_state) {
	case STATES::IDLE:
		switch (state) {
		case STATES::STOPPING:
			transition = STATE_TRANSITIONS::STOPPING_TO_IDLE;
			break;
		case STATES::IDLE:
			transition = STATE_TRANSITIONS::IDLE_TO_IDLE;
			break;
		default:
			TRACE_MSG(L"Received state IDLE after " << static_cast<int>(state));
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
	switch (transition) {
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
			catch (winrt::hresult_illegal_method_call const&) {
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
		break;
	}
}


void WinMTRDialog::OnTimer(UINT_PTR nIDEvent) noexcept
{
	static unsigned int call_count = 0;
	call_count += 1;
	//std::unique_lock lock(traceThreadMutex, std::try_to_lock);
	const bool is_tracing = tracing.load(std::memory_order_acquire);
	if (state == STATES::EXIT && !is_tracing) {
		OnOK();
	}

	if (!is_tracing) {
		Transit(STATES::IDLE);
	}
	else if ((call_count % 10 == 0)) {
		if (state == STATES::TRACING) Transit(STATES::TRACING);
		else if (state == STATES::STOPPING) Transit(STATES::STOPPING);
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
