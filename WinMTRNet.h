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
#include <cstdint>
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
  SOCKADDR_STORAGE addr;
  std::wstring name;
  int xmit;			// number of PING packets sent
  int returned;		// number of ICMP echo replies received
  unsigned long total;	// total time
  int last;				// last time
  int best;				// best time
  int worst;			// worst time
};

inline size_t getAddressSize(const sockaddr& addr) noexcept {
	return addr.sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

inline size_t getAddressSize(const SOCKADDR_STORAGE& addr) noexcept {
	return addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

inline bool isValidAddress(const sockaddr& addr) noexcept {
	return addr.sa_family == AF_INET || addr.sa_family == AF_INET6;
}

inline bool isValidAddress(const SOCKADDR_STORAGE& addr) noexcept {
	return addr.ss_family == AF_INET || addr.ss_family == AF_INET6;
}
struct trace_thread;

void TraceThread(trace_thread& current);

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

class WinMTRNet {

public:

	WinMTRNet(WinMTRDialog *wp);
	~WinMTRNet();
	void	DoTrace(sockaddr &address);
	void	ResetHops();
	void	StopTrace();

	SOCKADDR_STORAGE GetAddr(int at);
	std::wstring GetName(int at);
	int		GetBest(int at);
	int		GetWorst(int at);
	int		GetAvg(int at);
	int		GetPercent(int at);
	int		GetLast(int at);
	int		GetReturned(int at);
	int		GetXmit(int at);
	int		GetMax();

	void	SetAddr(int at, sockaddr& addr);
	void	SetName(int at, std::wstring n);
	void	SetBest(int at, int current);
	void	SetWorst(int at, int current);
	void	SetLast(int at, int last);
	void	AddReturned(int at);
	void	AddXmit(int at);

	
private:
	std::array<s_nethost, MaxHost>	host;
	SOCKADDR_STORAGE last_remote_addr;
	std::recursive_mutex	ghMutex;
	WinMTRDialog* wmtrdlg;
	HANDLE				hICMP;
	winmtr::helper::WSAHelper wsaHelper;
	std::atomic_bool	tracing;

	friend void TraceThread(trace_thread&);
};

#endif	// ifndef WINMTRNET_H_
