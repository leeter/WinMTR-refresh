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
// FILE:            WinMTRProperties.cpp
//
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRProperties.h"
import <format>;
import WinMTRUtils;

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//*****************************************************************************
// BEGIN_MESSAGE_MAP
//
// 
//*****************************************************************************
BEGIN_MESSAGE_MAP(WinMTRProperties, CDialog)
END_MESSAGE_MAP()


//*****************************************************************************
// WinMTRProperties::WinMTRProperties
//
// 
//*****************************************************************************
WinMTRProperties::WinMTRProperties(CWnd* pParent) noexcept : CDialog(WinMTRProperties::IDD, pParent)
,ping_last()
,ping_best()
,ping_avrg()
,ping_worst()
,pck_sent()
,pck_recv()
,pck_loss()
{
}


//*****************************************************************************
// WinMTRroperties::DoDataExchange
//
// 
//*****************************************************************************
void WinMTRProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PHOST, m_editHost);
	DDX_Control(pDX, IDC_EDIT_PIP, m_editIP);
	DDX_Control(pDX, IDC_EDIT_PCOMMENT, m_editComment);

	DDX_Control(pDX, IDC_EDIT_PLOSS, m_editLoss);
	DDX_Control(pDX, IDC_EDIT_PSENT, m_editSent);
	DDX_Control(pDX, IDC_EDIT_PRECV, m_editRecv);

	DDX_Control(pDX, IDC_EDIT_PLAST, m_editLast);
	DDX_Control(pDX, IDC_EDIT_PBEST, m_editBest);
	DDX_Control(pDX, IDC_EDIT_PWORST, m_editWorst);
	DDX_Control(pDX, IDC_EDIT_PAVRG, m_editAvrg);
}


//*****************************************************************************
// WinMTRProperties::OnInitDialog
//
// 
//*****************************************************************************
BOOL WinMTRProperties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	wchar_t buf[255] = {};
	
	m_editIP.SetWindowText(ip.c_str());
	m_editHost.SetWindowText(host.c_str());
	m_editComment.SetWindowText(comment.c_str());
	constexpr auto writable_size = std::size(buf) - 1;
	auto result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, pck_loss);
	*result.out = '\0';
	m_editLoss.SetWindowText(buf);
	result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, pck_sent);
	*result.out = '\0';
	m_editSent.SetWindowText(buf);
	result = std::format_to_n(buf, writable_size, WinMTRUtils::int_number_format, pck_recv);
	*result.out = '\0';
	m_editRecv.SetWindowText(buf);

	result = std::format_to_n(buf, writable_size, WinMTRUtils::float_number_format, ping_last);
	*result.out = '\0';
	m_editLast.SetWindowText(buf);
	result = std::format_to_n(buf, writable_size, WinMTRUtils::float_number_format, ping_best);
	*result.out = '\0';
	m_editBest.SetWindowText(buf);
	result = std::format_to_n(buf, writable_size, WinMTRUtils::float_number_format, ping_worst);
	*result.out = '\0';
	m_editWorst.SetWindowText(buf);
	result = std::format_to_n(buf, writable_size, WinMTRUtils::float_number_format, ping_avrg);
	*result.out = '\0';
	m_editAvrg.SetWindowText(buf);

	return FALSE;
}

