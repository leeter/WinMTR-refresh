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

class WinMTROptions final : public CDialog
{
public:
	WinMTROptions(CWnd* pParent = NULL);

	inline void SetUseDNS(bool udns) noexcept { useDNS = udns;  };
	inline void SetInterval(double i) noexcept { interval = i;   };
	inline void SetPingSize(int ps) noexcept { pingsize = ps;  };
	inline void SetMaxLRU(int mlru) noexcept { maxLRU = mlru;  };
	inline void SetUseIPv4(bool uip4) noexcept { useIPv4 = uip4; }
	inline void SetUseIPv6(bool uip6) noexcept { useIPv6 = uip6; }

	inline auto GetInterval() const noexcept { return interval;   };
	inline auto GetPingSize() const noexcept { return pingsize;   };
	inline auto GetMaxLRU()	const noexcept { return maxLRU;   };
	inline auto GetUseDNS() const noexcept { return useDNS; }
	inline auto GetUseIPv4() const noexcept { return useIPv4; }
	inline auto GetUseIPv6() const noexcept { return useIPv6; }

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
	bool     useDNS = false;
	bool	 useIPv4 = true;
	bool	 useIPv6 = true;
	
public:
	afx_msg void OnBnClickedIpv4Check();
	afx_msg void OnBnClickedUseipv6Check();
};

#endif // ifndef WINMTROPTIONS_H_
