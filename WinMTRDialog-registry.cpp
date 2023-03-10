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

module WinMTR.Dialog:registry;
import :ClassDef;

import <format>;
import <string_view>;
import WinMTRVerUtil;
import WinMTR.Options;

using namespace std::literals;
namespace {
	static constexpr auto reg_host_fmt = L"Host{:d}"sv;
	const auto NrLRU_REG_KEY = L"NrLRU";
	const auto config_key_name = LR"(Software\WinMTR\Config)";
	const auto lru_key_name = LR"(Software\WinMTR\LRU)";
}
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
		KEY_ALL_ACCESS) != ERROR_SUCCESS) {
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
	if (config_key.QueryDWORDValue(L"PingSize", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = pingsize;
		config_key.SetDWORDValue(L"PingSize", tmp_dword);
	}
	else {
		if (!hasPingsizeFromCmdLine) pingsize = tmp_dword;
	}

	if (config_key.QueryDWORDValue(L"MaxLRU", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = maxLRU;
		config_key.SetDWORDValue(L"MaxLRU", tmp_dword);
	}
	else {
		if (!hasMaxLRUFromCmdLine) maxLRU = tmp_dword;
	}

	if (config_key.QueryDWORDValue(L"UseDNS", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = useDNS ? 1 : 0;
		config_key.SetDWORDValue(L"UseDNS", tmp_dword);
	}
	else {
		if (!hasUseDNSFromCmdLine) useDNS = (BOOL)tmp_dword;
	}

	if (config_key.QueryDWORDValue(L"Interval", tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = static_cast<DWORD>(interval * 1000);
		config_key.SetDWORDValue(L"Interval", tmp_dword);
	}
	else {
		if (!hasIntervalFromCmdLine) interval = (float)tmp_dword / 1000.0;
	}
	CRegKey lru_key;
	if (lru_key.Create(versionKey,
		L"LRU",
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS) != ERROR_SUCCESS)
		return FALSE;
	if (lru_key.QueryDWORDValue(NrLRU_REG_KEY, tmp_dword) != ERROR_SUCCESS) {
		tmp_dword = nrLRU;
		lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);
	}
	else {
		wchar_t key_name[20];
		wchar_t str_host[NI_MAXHOST];
		nrLRU = tmp_dword;
		constexpr auto key_name_size = std::size(key_name) - 1;
		for (int i = 0; i < maxLRU; i++) {
			auto result = std::format_to_n(key_name, key_name_size, reg_host_fmt, i + 1);
			*result.out = '\0';
			auto value_size = static_cast<DWORD>(std::size(str_host));
			if (lru_key.QueryStringValue(key_name, str_host, &value_size) == ERROR_SUCCESS) {
				str_host[value_size] = L'\0';
				m_comboHost.AddString((CString)str_host);
			}
		}
	}
	m_comboHost.AddString(CString((LPCTSTR)IDS_STRING_CLEAR_HISTORY));
	return TRUE;
}

void WinMTRDialog::ClearHistory()
{
	DWORD tmp_dword;
	wchar_t key_name[20] = {};
	CRegKey lru_key;
	lru_key.Open(HKEY_CURRENT_USER, LR"(Software\WinMTR\LRU)", KEY_ALL_ACCESS);
	constexpr auto key_name_size = std::size(key_name) - 1;
	for (int i = 0; i <= nrLRU; i++) {
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


//*****************************************************************************
// WinMTRDialog::OnRestart
//
// 
//*****************************************************************************
void WinMTRDialog::OnRestart() noexcept
{
	// If clear history is selected, just clear the registry and listbox and return
	if (m_comboHost.GetCurSel() == m_comboHost.GetCount() - 1) {
		ClearHistory();
		return;
	}

	CString sHost;
	if (state == STATES::IDLE) {
		m_comboHost.GetWindowTextW(sHost);
		sHost.TrimLeft();
		sHost.TrimLeft();

		if (sHost.IsEmpty()) {
			AfxMessageBox(L"No host specified!");
			m_comboHost.SetFocus();
			return;
		}
		m_listMTR.DeleteAllItems();
	}

	if (state == STATES::IDLE) {

		if (InitMTRNet()) {
			if (m_comboHost.FindString(-1, sHost) == CB_ERR) {
				m_comboHost.InsertString(m_comboHost.GetCount() - 1, sHost);

				wchar_t key_name[20];
				CRegKey lru_key;
				lru_key.Open(HKEY_CURRENT_USER, lru_key_name, KEY_ALL_ACCESS);

				if (nrLRU >= maxLRU)
					nrLRU = 0;

				nrLRU++;
				auto result = std::format_to_n(key_name, std::size(key_name) - 1, reg_host_fmt, nrLRU);
				*result.out = '\0';
				lru_key.SetStringValue(key_name, static_cast<LPCWSTR>(sHost));
				auto tmp_dword = static_cast<DWORD>(nrLRU);
				lru_key.SetDWORDValue(NrLRU_REG_KEY, tmp_dword);
			}
			Transit(STATES::TRACING);
		}
	}
	else {
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

	if (IDOK == optDlg.DoModal()) {

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
		if (maxLRU < nrLRU) {
			CRegKey lru_key;
			lru_key.Open(HKEY_CURRENT_USER, lru_key_name, KEY_ALL_ACCESS);
			constexpr auto key_name_size = std::size(key_name) - 1;
			for (int i = maxLRU; i <= nrLRU; i++) {
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
