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

// WinMTRHelp.cpp : implementation file
//

#include "WinMTRGlobal.h"
#include "WinMTRHelp.h"
#include "afxdialogex.h"


// WinMTRHelp dialog

IMPLEMENT_DYNAMIC(WinMTRHelp, CDialog)

WinMTRHelp::WinMTRHelp(CWnd* pParent /*=NULL*/)
	: CDialog(WinMTRHelp::IDD, pParent)
{

}

WinMTRHelp::~WinMTRHelp()
{
}

void WinMTRHelp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(WinMTRHelp, CDialog)
	ON_BN_CLICKED(IDOK, &WinMTRHelp::OnBnClickedOk)
END_MESSAGE_MAP()


// WinMTRHelp message handlers


void WinMTRHelp::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
}
