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
#include <ws2tcpip.h>
#include "resource.h"

module WinMTR.Dialog:tracing;

import :ClassDef;
import WinMTRSNetHost;
import WinMTRDnsUtil;
import <stop_token>;
import <optional>;
import <mutex>;
import <format>;
import <string_view>;
import <winrt/Windows.Foundation.h>;

using namespace std::literals;

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
	}tracexit{ this };

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