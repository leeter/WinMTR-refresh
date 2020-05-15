//*****************************************************************************
// FILE:            WinMTRNet.cpp
//
//*****************************************************************************
#include "WinMTRGlobal.h"
#include "WinMTRNet.h"
#include "WinMTRDialog.h"
#include <iostream>
#include <sstream>
#include <iphlpapi.h>
#include <icmpapi.h>

#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugString(dbg_msg.str().c_str());				\
	}

#define MAX_HOPS				30

struct trace_thread {
	int			address;
	WinMTRNet	*winmtr;
	int			ttl;
};

struct dns_resolver_thread {
	int			index;
	WinMTRNet	*winmtr;
};


void DnsResolverThread(dns_resolver_thread dnt);


WinMTRNet::WinMTRNet(WinMTRDialog *wp)
	:host(),
	last_remote_addr(),
	wmtrdlg(wp),
	hICMP(INVALID_HANDLE_VALUE),
	wsaHelper(MAKEWORD(2, 2)),
	tracing(false){
	

    if(!wsaHelper) {
        AfxMessageBox(L"Failed initializing windows sockets library!");
		return;
    }

    /*
     * IcmpCreateFile() - Open the ping service
     */
    hICMP = IcmpCreateFile();
    if (hICMP == INVALID_HANDLE_VALUE) {
        AfxMessageBox(L"Error in ICMP.DLL !");
        return;
    }
}

WinMTRNet::~WinMTRNet()
{
	if(hICMP != INVALID_HANDLE_VALUE) {
		/*
		 * IcmpCloseHandle - Close the ICMP handle
		 */
		IcmpCloseHandle(hICMP);
	}
}

void WinMTRNet::ResetHops()
{
	for(auto & host : this->host) {
		host = s_nethost();
	}
}

void WinMTRNet::DoTrace(int address)
{
	std::vector<std::thread> threads;
	threads.reserve(MAX_HOPS);
	tracing = true;

	ResetHops();
	sockaddr_in* addr = reinterpret_cast<sockaddr_in*>((&last_remote_addr));
	addr->sin_addr.S_un.S_addr = address;

	// one thread per TTL value
	for(int i = 0; i < MAX_HOPS; i++) {
		trace_thread current{};
		current.address = address;
		current.winmtr = this;
		current.ttl = i + 1;
		threads.emplace_back([](auto trd) {
			TraceThread(trd);
		}, current);
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

void WinMTRNet::StopTrace()
{
	tracing = false;
}

void TraceThread(trace_thread& current)
{
	using namespace std::chrono_literals;
	using namespace std::string_literals;
	WinMTRNet *wmtrnet = current.winmtr;
	TRACE_MSG(L"Threaad with TTL=" << current.ttl << L" started.");

    
	std::vector<char>	achReqData(static_cast<size_t>(wmtrnet->wmtrdlg->pingsize) + 8192); //whitespaces
	int				nDataLen									= wmtrnet->wmtrdlg->pingsize;
	std::vector<char> achRepData(sizeof(ICMPECHO) + 8192);


    /*
     * Init IPInfo structure
	 */
	IP_OPTION_INFORMATION	stIPInfo = {};
    stIPInfo.Ttl			= current.ttl;
    stIPInfo.Flags			= IP_FLAG_DF;

    for (int i=0; i<nDataLen; i++) achReqData[i] = 32; 

    while(wmtrnet->tracing) {
	    
		// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
		// Check if the current thread should be closed
		if( current.ttl > wmtrnet->GetMax() ) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss
		auto dwReplyCount = IcmpSendEcho(wmtrnet->hICMP, current.address, achReqData.data(), nDataLen, &stIPInfo, achRepData.data(), achRepData.size(), ECHO_REPLY_TIMEOUT);

		PICMPECHO icmp_echo_reply = (PICMPECHO)achRepData.data();
		
		wmtrnet->AddXmit(current.ttl - 1);
		if (dwReplyCount != 0) {
			TRACE_MSG(L"TTL " << current.ttl << L" reply TTL " << icmp_echo_reply->Options.Ttl << L" Status " << icmp_echo_reply->Status << L" Reply count " << dwReplyCount);

			switch(icmp_echo_reply->Status) {
				case IP_SUCCESS:
				case IP_TTL_EXPIRED_TRANSIT:
					wmtrnet->SetLast(current.ttl - 1, icmp_echo_reply->RoundTripTime);
					wmtrnet->SetBest(current.ttl - 1, icmp_echo_reply->RoundTripTime);
					wmtrnet->SetWorst(current.ttl - 1, icmp_echo_reply->RoundTripTime);
					wmtrnet->AddReturned(current.ttl - 1);
					{
						sockaddr_in naddr = {};
						naddr.sin_family = AF_INET;
						naddr.sin_addr.S_un.S_addr = icmp_echo_reply->Address;
						wmtrnet->SetAddr(current.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
					}
				break;
				case IP_BUF_TOO_SMALL:
					wmtrnet->SetName(current.ttl - 1, L"Reply buffer too small."s);
				break;
				case IP_DEST_NET_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, L"Destination network unreachable."s);
				break;
				case IP_DEST_HOST_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, L"Destination host unreachable."s);
				break;
				case IP_DEST_PROT_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, L"Destination protocol unreachable."s);
				break;
				case IP_DEST_PORT_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, L"Destination port unreachable."s);
				break;
				case IP_NO_RESOURCES:
					wmtrnet->SetName(current.ttl - 1, L"Insufficient IP resources were available."s);
				break;
				case IP_BAD_OPTION:
					wmtrnet->SetName(current.ttl - 1, L"Bad IP option was specified."s);
				break;
				case IP_HW_ERROR:
					wmtrnet->SetName(current.ttl - 1, L"Hardware error occurred."s);
				break;
				case IP_PACKET_TOO_BIG:
					wmtrnet->SetName(current.ttl - 1, L"Packet was too big."s);
				break;
				case IP_REQ_TIMED_OUT:
					wmtrnet->SetName(current.ttl - 1, L"Request timed out."s);
				break;
				case IP_BAD_REQ:
					wmtrnet->SetName(current.ttl - 1, L"Bad request."s);
				break;
				case IP_BAD_ROUTE:
					wmtrnet->SetName(current.ttl - 1, L"Bad route."s);
				break;
				case IP_TTL_EXPIRED_REASSEM:
					wmtrnet->SetName(current.ttl - 1, L"The time to live expired during fragment reassembly."s);
				break;
				case IP_PARAM_PROBLEM:
					wmtrnet->SetName(current.ttl - 1, L"Parameter problem."s);
				break;
				case IP_SOURCE_QUENCH:
					wmtrnet->SetName(current.ttl - 1, L"Datagrams are arriving too fast to be processed and datagrams may have been discarded."s);
				break;
				case IP_OPTION_TOO_BIG:
					wmtrnet->SetName(current.ttl - 1, L"An IP option was too big."s);
				break;
				case IP_BAD_DESTINATION:
					wmtrnet->SetName(current.ttl - 1, L"Bad destination."s);
				break;
				case IP_GENERAL_FAILURE:
					wmtrnet->SetName(current.ttl - 1, L"General failure."s);
				break;
				default:
					wmtrnet->SetName(current.ttl - 1, L"General failure."s);
			}
			const auto intervalInSec = wmtrnet->wmtrdlg->interval * 1s;
			const auto roundTripDuration = std::chrono::milliseconds(icmp_echo_reply->RoundTripTime);
			if (intervalInSec > roundTripDuration) {
				const auto sleepTime = intervalInSec - roundTripDuration;
				std::this_thread::sleep_for(sleepTime);
			}
		}

    } /* end ping loop */

	TRACE_MSG(L"Thread with TTL=" << current.ttl << L" stopped.");
}

sockaddr_storage WinMTRNet::GetAddr(int at)
{
	std::unique_lock lock(ghMutex);
	return host[at].addr;
}

std::wstring WinMTRNet::GetName(int at)
{
	std::unique_lock lock(ghMutex);
	if (host[at].name.empty()) {
		SOCKADDR_STORAGE addr = GetAddr(at);
		lock.unlock();
		if (!isValidAddress(addr)) {
			return {};
		}
		std::wstring out;
		out.resize(40);
		auto addrlen = getAddressSize(addr);
		DWORD addrstrsize = out.size();
		auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), addrlen, nullptr, out.data(), &addrstrsize);
		if (!result) {
			out.resize(addrstrsize - 1);
		}
		
		return out;
	}
	return host[at].name;
}

int WinMTRNet::GetBest(int at)
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].best;
	return ret;
}

int WinMTRNet::GetWorst(int at)
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].worst;
	return ret;
}

int WinMTRNet::GetAvg(int at)
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].returned == 0 ? 0 : host[at].total / host[at].returned;
	return ret;
}

int WinMTRNet::GetPercent(int at)
{
	std::unique_lock lock(ghMutex);
	int ret = (host[at].xmit == 0) ? 0 : (100 - (100 * host[at].returned / host[at].xmit));
	return ret;
}

int WinMTRNet::GetLast(int at)
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].last;
	return ret;
}

int WinMTRNet::GetReturned(int at)
{ 
	std::unique_lock lock(ghMutex);
	int ret = host[at].returned;
	return ret;
}

int WinMTRNet::GetXmit(int at)
{ 
	std::unique_lock lock(ghMutex);
	int ret = host[at].xmit;
	return ret;
}

int WinMTRNet::GetMax()
{
	std::unique_lock lock(ghMutex);
	int max = MAX_HOPS;

	// first match: traced address responds on ping requests, and the address is in the hosts list
	for(int i = 0; i < MAX_HOPS; i++) {
		if(memcmp(&host[i].addr, &last_remote_addr, sizeof(SOCKADDR_STORAGE)) == 0) {
			max = i + 1;
			break;
		}
	}

	// second match:  traced address doesn't responds on ping requests
	if(max == MAX_HOPS) {
		while((max > 1) && (memcmp(&host[max - 1].addr, &host[max - 2].addr, sizeof(SOCKADDR_STORAGE)) == 0 && isValidAddress(host[max - 1].addr)) ) max--;
	}
	return max;
}



void WinMTRNet::SetAddr(int at, sockaddr& addr)
{
	std::unique_lock lock(ghMutex);
	if(!isValidAddress(host[at].addr) && isValidAddress(addr)) {
		//TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
		memcpy(&host[at].addr, &addr, getAddressSize(addr));
		dns_resolver_thread dnt;
		dnt.index = at;
		dnt.winmtr = this;
		if (wmtrdlg->useDNS) {
			auto dnsThread = std::thread(DnsResolverThread, dnt);
			dnsThread.detach();
		}
	}
}

void WinMTRNet::SetName(int at, std::wstring n)
{
	std::unique_lock lock(ghMutex);
	host[at].name = std::move(n);
}

void WinMTRNet::SetBest(int at, int current)
{
	std::unique_lock lock(ghMutex);
	if(host[at].best > current || host[at].xmit == 1) {
		host[at].best = current;
	};
	if(host[at].worst < current) {
		host[at].worst = current;
	}
}

void WinMTRNet::SetWorst(int at, int current)
{
	//std::unique_lock lock(ghMutex);
}

void WinMTRNet::SetLast(int at, int last)
{
	std::unique_lock lock(ghMutex);
	host[at].last = last;
	host[at].total += last;
}

void WinMTRNet::AddReturned(int at)
{
	std::unique_lock lock(ghMutex);
	host[at].returned++;
}

void WinMTRNet::AddXmit(int at)
{
	std::unique_lock lock(ghMutex);
	host[at].xmit++;
}

void DnsResolverThread(dns_resolver_thread dnt)
{
	TRACE_MSG(L"DNS resolver thread started.");
	WinMTRNet* wn = dnt.winmtr;

	wchar_t buf[NI_MAXHOST];
	sockaddr_storage addr = wn->GetAddr(dnt.index);
	
	auto nresult = GetNameInfoW(reinterpret_cast<sockaddr*>(&addr), getAddressSize(addr), buf, sizeof(buf), nullptr, 0, 0);

	if(!nresult) {
		wn->SetName(dnt.index, buf);
	} else {
		std::wstring out;
		out.resize(40);
		auto addrlen = getAddressSize(addr);
		DWORD addrstrsize = out.size();
		auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), addrlen, nullptr, out.data(), &addrstrsize);
		if (!result) {
			out.resize(addrstrsize - 1);
		}
		wn->SetName(dnt.index, std::move(out));
	}
	
	TRACE_MSG(L"DNS resolver thread stopped.");
}


