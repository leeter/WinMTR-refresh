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

