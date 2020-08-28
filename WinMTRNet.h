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
#include <vector>
#include "WinMTRWSAhelper.h"


class WinMTRDialog;

struct s_nethost {
  SOCKADDR_STORAGE addr = {};
  std::wstring name;
  int xmit = 0;			// number of PING packets sent
  int returned = 0;		// number of ICMP echo replies received
  unsigned long total = 0;	// total time
  int last = 0;				// last time
  int best = 0;				// best time
  int worst = 0;			// worst time
};

template<typename T>
inline constexpr auto getAddressFamily(const T& addr) noexcept {
	if constexpr (std::is_same_v<sockaddr, T>) {
		return addr.sa_family;
	}
	else if (std::is_same_v<SOCKADDR_STORAGE, T>) {
		return addr.ss_family;
	}
}

template<typename T>
inline constexpr size_t getAddressSize(const T& addr) noexcept {
	return getAddressFamily(addr) == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

template<typename T>
inline constexpr bool isValidAddress(const T& addr) noexcept {
	const ADDRESS_FAMILY family = getAddressFamily(addr);
	
	return (family == AF_INET || family == AF_INET6);
}
struct trace_thread;

//*****************************************************************************
// CLASS:  WinMTRNet
//
//
//*****************************************************************************

class WinMTRNet {

public:

	WinMTRNet(WinMTRDialog *wp);
	~WinMTRNet() noexcept;
	void	DoTrace(sockaddr &address);
	void	ResetHops();
	void	StopTrace() noexcept;

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
	double	GetInterval() const noexcept;
	
	bool	GetIsTracing() const noexcept {
		return tracing;
	}
	int	GetPingSize() const noexcept;

	void	SetAddr(int at, sockaddr& addr);
	void	SetName(int at, std::wstring n);
	void	SetBest(int at, int current);
	void	SetWorst(int at, int current);
	void	SetLast(int at, int last);
	void	AddReturned(int at);
	void	AddXmit(int at);

	
private:
	std::array<s_nethost, MaxHost>	host;
	std::vector<std::unique_ptr<trace_thread>> traces;
	SOCKADDR_STORAGE last_remote_addr;
	std::recursive_mutex	ghMutex;
	WinMTRDialog* wmtrdlg;
	winmtr::helper::WSAHelper wsaHelper;
	std::atomic_bool	tracing;
	void handleICMPv4(trace_thread& current);
	void handleICMPv6(trace_thread& current);
	void handleDefault(const trace_thread& current, ULONG status);
	void sleepTilInterval(ULONG roundTripTime);
};

#endif	// ifndef WINMTRNET_H_
