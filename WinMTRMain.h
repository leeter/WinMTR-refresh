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
// FILE:            WinMTRMain.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************

#ifndef WINMTRMAIN_H_
#define WINMTRMAIN_H_

#include "WinMTRDialog.h"
#include <string>
#include <optional>


//*****************************************************************************
// CLASS:  WinMTRMain
//
//
//*****************************************************************************

class WinMTRMain : public CWinApp
{
public:
	WinMTRMain();

	virtual BOOL InitInstance() override final;

	DECLARE_MESSAGE_MAP()

private:
	/*void	ParseCommandLineParams(LPTSTR cmd, WinMTRDialog *wmtrdlg);
	int		GetParamValue(LPTSTR cmd, const wchar_t * param, wchar_t sparam, wchar_t *value);
	std::optional<std::wstring>		GetHostNameParamValue(LPTSTR cmd);*/

};

#endif // ifndef WINMTRMAIN_H_

