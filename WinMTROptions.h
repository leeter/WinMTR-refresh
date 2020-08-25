//*****************************************************************************
// FILE:            WinMTROptions.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************

#ifndef WINMTROPTIONS_H_
#define WINMTROPTIONS_H_


#include "resource.h"

//*****************************************************************************
// CLASS:  WinMTROptions
//
//
//*****************************************************************************

class WinMTROptions : public CDialog
{
public:
	WinMTROptions(CWnd* pParent = NULL);

	void SetUseDNS(BOOL udns)		{ useDNS = udns;  };
	void SetInterval(double i)		{ interval = i;   };
	void SetPingSize(int ps)		{ pingsize = ps;  };
	void SetMaxLRU(int mlru)		{ maxLRU = mlru;  };

	double GetInterval()			{ return interval;   };
	int GetPingSize()				{ return pingsize;   };
	int GetMaxLRU()					{ return maxLRU;   };
	BOOL GetUseDNS()				{ return useDNS;     };

	enum { IDD = IDD_DIALOG_OPTIONS };
	CEdit	m_editSize;
	CEdit	m_editInterval;
	CEdit	m_editMaxLRU;
	CButton	m_checkDNS;
	CButton m_useIPv4;
	CButton m_useIPv6;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnLicense();
	DECLARE_MESSAGE_MAP()
	
private:
	double   interval = 0.0;
	int      pingsize = 0;
	int		 maxLRU = 0;
	BOOL     useDNS = FALSE;
	bool	 useIPv4 = true;
	bool	 useIPv6 = true;
	
public:
	afx_msg void OnBnClickedIpv4Check();
	afx_msg void OnBnClickedUseipv6Check();
};

#endif // ifndef WINMTROPTIONS_H_
