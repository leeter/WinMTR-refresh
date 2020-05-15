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
	wmtrdlg(wp),
	hICMP(INVALID_HANDLE_VALUE),
	last_remote_addr(0),
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

	last_remote_addr = address;

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
					wmtrnet->SetAddr(current.ttl - 1, icmp_echo_reply->Address);
				break;
				case IP_BUF_TOO_SMALL:
					wmtrnet->SetName(current.ttl - 1, "Reply buffer too small.");
				break;
				case IP_DEST_NET_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, "Destination network unreachable.");
				break;
				case IP_DEST_HOST_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, "Destination host unreachable.");
				break;
				case IP_DEST_PROT_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, "Destination protocol unreachable.");
				break;
				case IP_DEST_PORT_UNREACHABLE:
					wmtrnet->SetName(current.ttl - 1, "Destination port unreachable.");
				break;
				case IP_NO_RESOURCES:
					wmtrnet->SetName(current.ttl - 1, "Insufficient IP resources were available.");
				break;
				case IP_BAD_OPTION:
					wmtrnet->SetName(current.ttl - 1, "Bad IP option was specified.");
				break;
				case IP_HW_ERROR:
					wmtrnet->SetName(current.ttl - 1, "Hardware error occurred.");
				break;
				case IP_PACKET_TOO_BIG:
					wmtrnet->SetName(current.ttl - 1, "Packet was too big.");
				break;
				case IP_REQ_TIMED_OUT:
					wmtrnet->SetName(current.ttl - 1, "Request timed out.");
				break;
				case IP_BAD_REQ:
					wmtrnet->SetName(current.ttl - 1, "Bad request.");
				break;
				case IP_BAD_ROUTE:
					wmtrnet->SetName(current.ttl - 1, "Bad route.");
				break;
				case IP_TTL_EXPIRED_REASSEM:
					wmtrnet->SetName(current.ttl - 1, "The time to live expired during fragment reassembly.");
				break;
				case IP_PARAM_PROBLEM:
					wmtrnet->SetName(current.ttl - 1, "Parameter problem.");
				break;
				case IP_SOURCE_QUENCH:
					wmtrnet->SetName(current.ttl - 1, "Datagrams are arriving too fast to be processed and datagrams may have been discarded.");
				break;
				case IP_OPTION_TOO_BIG:
					wmtrnet->SetName(current.ttl - 1, "An IP option was too big.");
				break;
				case IP_BAD_DESTINATION:
					wmtrnet->SetName(current.ttl - 1, "Bad destination.");
				break;
				case IP_GENERAL_FAILURE:
					wmtrnet->SetName(current.ttl - 1, "General failure.");
				break;
				default:
					wmtrnet->SetName(current.ttl - 1, "General failure.");
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

int WinMTRNet::GetAddr(int at)
{
	std::unique_lock lock(ghMutex);
	int addr = ntohl(host[at].addr);
	return addr;
}

std::wstring WinMTRNet::GetName(int at)
{
	std::unique_lock lock(ghMutex);
	if(!strcmp(host[at].name, "")) {
		int addr = GetAddr(at);
		lock.unlock();
		if (addr == 0) {
			return {};
		}
		std::wostringstream out;
		out << ((addr >> 24) & 0xff) << L'.' <<
			((addr >> 16) & 0xff) << L'.' << 
			((addr >> 8) & 0xff) << L'.' << (addr & 0xff);
		
		return out.str();
	}
	std::wostringstream out;
	out << host[at].name;
	return out.str();
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
		if(host[i].addr == last_remote_addr) {
			max = i + 1;
			break;
		}
	}

	// second match:  traced address doesn't responds on ping requests
	if(max == MAX_HOPS) {
		while((max > 1) && (host[max - 1].addr == host[max - 2].addr) && (host[max - 1].addr != 0) ) max--;
	}
	return max;
}

void WinMTRNet::SetAddr(int at, __int32 addr)
{
	std::unique_lock lock(ghMutex);
	if(host[at].addr == 0 && addr != 0) {
		TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
		host[at].addr = addr;
		dns_resolver_thread dnt;
		dnt.index = at;
		dnt.winmtr = this;
		if (wmtrdlg->useDNS) {
			auto dnsThread = std::thread(DnsResolverThread, dnt);
			dnsThread.detach();
		}
	}
}

void WinMTRNet::SetName(int at, char *n)
{
	std::unique_lock lock(ghMutex);
	strcpy(host[at].name, n);
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

	struct hostent *phent ;

	char buf[100];
	int addr = wn->GetAddr(dnt.index);
	sprintf (buf, "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);

	int haddr = htonl(addr);
	phent = gethostbyaddr( (const char*)&haddr, sizeof(int), AF_INET);

	if(phent) {
		wn->SetName(dnt.index, phent->h_name);
	} else {
		wn->SetName(dnt.index, buf);
	}
	
	TRACE_MSG("DNS resolver thread stopped.");
}


