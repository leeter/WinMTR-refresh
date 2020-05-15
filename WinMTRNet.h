//*****************************************************************************
// FILE:            WinMTRNet.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************

#ifndef WINMTRNET_H_
#define WINMTRNET_H_
#include <string>
#include <atomic>
#include <mutex>
#include <array>
#include "WinMTRWSAhelper.h"


class WinMTRDialog;

#ifdef _WIN64
typedef IP_OPTION_INFORMATION32 IPINFO, * PIPINFO, FAR* LPIPINFO;
#else
typedef IP_OPTION_INFORMATION IPINFO, * PIPINFO, FAR* LPIPINFO;
#endif



#ifdef _WIN64
typedef icmp_echo_reply32 ICMPECHO, *PICMPECHO, FAR *LPICMPECHO;
#else
typedef icmp_echo_reply ICMPECHO, *PICMPECHO, FAR *LPICMPECHO;
#endif

#define ECHO_REPLY_TIMEOUT 5000

struct s_nethost {
  __int32 addr;		// IP as a decimal, big endian
  int xmit;			// number of PING packets sent
  int returned;		// number of ICMP echo replies received
  unsigned long total;	// total time
  int last;				// last time
  int best;				// best time
  int worst;			// worst time
  char name[255];
};

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

class WinMTRNet {

public:

	WinMTRNet(WinMTRDialog *wp);
	~WinMTRNet();
	void	DoTrace(int address);
	void	ResetHops();
	void	StopTrace();

	int		GetAddr(int at);
	std::wstring GetName(int at);
	int		GetBest(int at);
	int		GetWorst(int at);
	int		GetAvg(int at);
	int		GetPercent(int at);
	int		GetLast(int at);
	int		GetReturned(int at);
	int		GetXmit(int at);
	int		GetMax();

	void	SetAddr(int at, __int32 addr);
	void	SetName(int at, char *n);
	void	SetBest(int at, int current);
	void	SetWorst(int at, int current);
	void	SetLast(int at, int last);
	void	AddReturned(int at);
	void	AddXmit(int at);

	WinMTRDialog		*wmtrdlg;
	__int32				last_remote_addr;
	std::atomic_bool	tracing;
	bool				initialized;
    HANDLE				hICMP;
private:

    std::array<s_nethost, MaxHost>	host;
	std::recursive_mutex	ghMutex; 
	winmtr::helper::WSAHelper wsaHelper;
};

#endif	// ifndef WINMTRNET_H_
