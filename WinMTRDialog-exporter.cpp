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
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Foundation.Diagnostics.h>
module WinMTR.Dialog:exporter;

import :ClassDef;
import <string>;
import <sstream>;
import <string_view>;
import <iterator>;
import <format>;
import <fstream>;

import WinMTR.Net;
import WinMTRSNetHost;
using namespace std::literals;
namespace {
	[[nodiscard]]
	std::wstring makeTextOutput(const WinMTRNet& wmtrnet) {
		std::wostringstream out_buf;
		out_buf << L"|-------------------------------------------------------------------------------------------|\r\n" \
			L"|                                      WinMTR statistics                                    |\r\n" \
			L"|                       Host              -   %%  | Sent | Recv | Best | Avrg | Wrst | Last |\r\n" \
			L"|-------------------------------------------------|------|------|------|------|------|------|\r\n"sv;
		std::ostream_iterator<wchar_t, wchar_t> out(out_buf);
		CString noResponse;
		noResponse.LoadStringW(IDS_STRING_NO_RESPONSE_FROM_HOST);

		for (const auto curr_state = wmtrnet.getCurrentState(); const auto & hop : curr_state) {
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
		out << L"<table>" \
			L"<thead><tr><th>Host</th><th>%%</th><th>Sent</th><th>Recv</th><th>Best</th><th>Avrg</th><th>Wrst</th><th>Last</th></tr></thead><tbody>"sv;
		std::ostream_iterator<wchar_t, wchar_t> outitr(out);

		CString noResponse;
		noResponse.LoadStringW(IDS_STRING_NO_RESPONSE_FROM_HOST);

		for (const auto curr_state = wmtrnet.getCurrentState(); const auto & hop : curr_state) {
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

		out << L"</tbody></table>"sv;
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
	if (dlg.DoModal() == IDOK) {
		const auto f_buf = makeTextOutput(*wmtrnet);

		if (std::wfstream fp(dlg.GetPathName(), std::ios::binary | std::ios::out | std::ios::trunc); fp) {
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

	if (dlg.DoModal() == IDOK) {
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

