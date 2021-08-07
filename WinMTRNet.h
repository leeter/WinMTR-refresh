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
constexpr auto MAX_HOPS = 30;

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

class WinMTRNet final: public std::enable_shared_from_this<WinMTRNet>{
	WinMTRNet(const WinMTRNet&) = delete;
	WinMTRNet& operator=(const WinMTRNet&) = delete;
public:

	WinMTRNet(const WinMTRDialog *wp);
	~WinMTRNet() noexcept;
	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction	DoTrace(sockaddr& address);
	void	ResetHops() noexcept;
	//winrt::fire_and_forget	StopTrace() noexcept;

	SOCKADDR_STORAGE GetAddr(int at) const;
	std::wstring GetName(int at);
	int		GetBest(int at) const;
	int		GetWorst(int at) const;
	int		GetAvg(int at) const;
	int		GetPercent(int at) const;
	int		GetLast(int at) const;
	int		GetReturned(int at) const;
	int		GetXmit(int at) const;
	int		GetMax() const;

private:
	std::array<s_nethost, MAX_HOPS>	host;
	SOCKADDR_STORAGE last_remote_addr;
	mutable std::recursive_mutex	ghMutex;
	const WinMTRDialog* wmtrdlg;
	winmtr::helper::WSAHelper wsaHelper;
	std::atomic_bool	tracing;
	std::optional<winrt::Windows::Foundation::IAsyncAction> tracer;
	std::optional<winrt::apartment_context> context;

	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction handleICMPv4(trace_thread current);
	[[nodiscard("The task should be awaited")]]
	winrt::Windows::Foundation::IAsyncAction handleICMPv6(trace_thread current);
	void handleDefault(const trace_thread& current, ULONG status);
	[[nodiscard("The task should be awaited")]] concurrency::task<void> sleepTilInterval(ULONG roundTripTime);

	void	SetAddr(int at, sockaddr& addr);
	void	SetName(int at, std::wstring n);
	void	SetBest(int at, int current);
	void	SetWorst(int at, int current);
	void	SetLast(int at, int last);
	void	AddReturned(int at);
	void	AddXmit(int at);
};

#endif	// ifndef WINMTRNET_H_
