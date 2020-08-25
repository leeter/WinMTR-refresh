//*****************************************************************************
// FILE:            WinMTRNet.cpp
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRNet.h"
#include "WinMTRDialog.h"
#include <wrl/wrappers/corewrappers.h>


namespace wrl = ::Microsoft::WRL;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI* PIO_APC_ROUTINE) (
	IN PVOID ApcContext,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG Reserved
	);
#include <iphlpapi.h>
#include <icmpapi.h>


#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugString(dbg_msg.str().c_str());				\
	}

#define MAX_HOPS				30

namespace {
	struct ICMPHandleTraits : wrl::Wrappers::HandleTraits::HANDLETraits {
		inline static bool Close(_In_ Type h) noexcept
		{
			return ::IcmpCloseHandle(h) != FALSE;
		}

		[[nodiscard]]
		inline static Type GetInvalidValue() noexcept
		{
			return INVALID_HANDLE_VALUE;
		}
	};

	using IcmpHandle = wrl::Wrappers::HandleT<ICMPHandleTraits>;
}

using namespace std::chrono_literals;
struct trace_thread {
	trace_thread(ADDRESS_FAMILY af, WinMTRNet* winmtr, UCHAR ttl)
		:
		address(),
		winmtr(winmtr),
		//timer(static_cast<unsigned int>((winmtr->GetInterval() * 1000ms).count()), this, nullptr, true),
		ttl(ttl)
	{
		if (af == AF_INET) {
			icmpHandle.Attach(IcmpCreateFile());
		}
		else if (af == AF_INET6) {
			icmpHandle.Attach(Icmp6CreateFile());
		}
	}
	SOCKADDR_STORAGE address;
	IcmpHandle icmpHandle;
	WinMTRNet* winmtr;
	UCHAR		ttl;

};


WinMTRNet::WinMTRNet(WinMTRDialog* wp)
	:host(),
	last_remote_addr(),
	wmtrdlg(wp),
	wsaHelper(MAKEWORD(2, 2)),
	tracing(false) {

	traces.reserve(MAX_HOPS);

	if (!wsaHelper) {
		AfxMessageBox(L"Failed initializing windows sockets library!");
		return;
	}
}

WinMTRNet::~WinMTRNet()
{}

void WinMTRNet::ResetHops()
{
	for (auto& host : this->host) {
		host = s_nethost();
	}
}

void WinMTRNet::handleDefault(const trace_thread& current, ULONG status) {
	using namespace std::string_literals;
	switch (status) {
	case IP_BUF_TOO_SMALL:
		this->SetName(current.ttl - 1, L"Reply buffer too small."s);
		break;
	case IP_DEST_NET_UNREACHABLE:
		this->SetName(current.ttl - 1, L"Destination network unreachable."s);
		break;
	case IP_DEST_HOST_UNREACHABLE:
		this->SetName(current.ttl - 1, L"Destination host unreachable."s);
		break;
	case IP_DEST_PROT_UNREACHABLE:
		this->SetName(current.ttl - 1, L"Destination protocol unreachable."s);
		break;
	case IP_DEST_PORT_UNREACHABLE:
		this->SetName(current.ttl - 1, L"Destination port unreachable."s);
		break;
	case IP_NO_RESOURCES:
		this->SetName(current.ttl - 1, L"Insufficient IP resources were available."s);
		break;
	case IP_BAD_OPTION:
		this->SetName(current.ttl - 1, L"Bad IP option was specified."s);
		break;
	case IP_HW_ERROR:
		this->SetName(current.ttl - 1, L"Hardware error occurred."s);
		break;
	case IP_PACKET_TOO_BIG:
		this->SetName(current.ttl - 1, L"Packet was too big."s);
		break;
	case IP_REQ_TIMED_OUT:
		this->SetName(current.ttl - 1, L"Request timed out."s);
		break;
	case IP_BAD_REQ:
		this->SetName(current.ttl - 1, L"Bad request."s);
		break;
	case IP_BAD_ROUTE:
		this->SetName(current.ttl - 1, L"Bad route."s);
		break;
	case IP_TTL_EXPIRED_REASSEM:
		this->SetName(current.ttl - 1, L"The time to live expired during fragment reassembly."s);
		break;
	case IP_PARAM_PROBLEM:
		this->SetName(current.ttl - 1, L"Parameter problem."s);
		break;
	case IP_SOURCE_QUENCH:
		this->SetName(current.ttl - 1, L"Datagrams are arriving too fast to be processed and datagrams may have been discarded."s);
		break;
	case IP_OPTION_TOO_BIG:
		this->SetName(current.ttl - 1, L"An IP option was too big."s);
		break;
	case IP_BAD_DESTINATION:
		this->SetName(current.ttl - 1, L"Bad destination."s);
		break;
	case IP_GENERAL_FAILURE:
		this->SetName(current.ttl - 1, L"General failure."s);
		break;
	default:
		this->SetName(current.ttl - 1, L"General failure."s);
	}
}

void WinMTRNet::sleepTilInterval(ULONG roundTripTime) {
	using namespace std::chrono_literals;
	const auto intervalInSec = this->GetInterval() * 1s;
	const auto roundTripDuration = std::chrono::milliseconds();
	if (intervalInSec > roundTripDuration) {
		const auto sleepTime = intervalInSec - roundTripDuration;
		std::this_thread::sleep_for(sleepTime);
	}
}


void WinMTRNet::handleICMPv6(trace_thread& current) {
	using namespace std::string_view_literals;
	std::vector<char>	achReqData(static_cast<size_t>(this->wmtrdlg->pingsize) + 8192); //whitespaces
	int				nDataLen = this->wmtrdlg->pingsize;
	std::vector<char> achRepData(sizeof(ICMPV6_ECHO_REPLY) + 8192);

	/*
	 * Init IPInfo structure
	 */
	IP_OPTION_INFORMATION	stIPInfo = {};
	stIPInfo.Ttl = current.ttl;
	stIPInfo.Flags = IP_FLAG_DF;
	SOCKADDR_IN6* addr = reinterpret_cast<SOCKADDR_IN6*>(&current.address);


	while (this->GetIsTracing()) {

		// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
		// Check if the current thread should be closed
		if (current.ttl > this->GetMax()) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss
		SOCKADDR_IN6 local = { .sin6_family = AF_INET6 };
		const auto dwReplyCount = Icmp6SendEcho2(current.icmpHandle.Get(), nullptr, nullptr, nullptr, &local, addr, achReqData.data(), nDataLen, &stIPInfo, achRepData.data(), gsl::narrow_cast<DWORD>(achRepData.size()), ECHO_REPLY_TIMEOUT);

		this->AddXmit(current.ttl - 1);
		if (dwReplyCount != 0) {

			PICMPV6_ECHO_REPLY icmp_echo_reply = (PICMPV6_ECHO_REPLY)achRepData.data();
			TRACE_MSG(L"TTL "sv << current.ttl << L" Status "sv << icmp_echo_reply->Status << L" Reply count "sv << dwReplyCount);
			switch (icmp_echo_reply->Status) {
			case IP_SUCCESS:
			case IP_TTL_EXPIRED_TRANSIT:
				this->SetLast(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetBest(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetWorst(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->AddReturned(current.ttl - 1);
				{
					sockaddr_in6 naddr = { .sin6_family = AF_INET6 };
					memcpy(&naddr.sin6_addr, icmp_echo_reply->Address.sin6_addr, sizeof(naddr.sin6_addr));
					this->SetAddr(current.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
				}
				break;
			default:
				this->handleDefault(current, icmp_echo_reply->Status);
				break;
			}
			this->sleepTilInterval(icmp_echo_reply->RoundTripTime);
		}
	}
}

void WinMTRNet::handleICMPv4(trace_thread& current) {
	using namespace std::string_view_literals;
	std::vector<std::byte>	achReqData(static_cast<size_t>(this->wmtrdlg->pingsize) + 8192); //whitespaces
	int				nDataLen = this->wmtrdlg->pingsize;
	std::vector<std::byte> achRepData(sizeof(ICMPECHO) + 8192);


	/*
	 * Init IPInfo structure
	 */
	IP_OPTION_INFORMATION	stIPInfo = {};
	stIPInfo.Ttl = current.ttl;
	stIPInfo.Flags = IP_FLAG_DF;
	std::fill_n(std::begin(achReqData), nDataLen, static_cast<std::byte>(32));
	//for (int i = 0; i < nDataLen; i++) achReqData[i] = 32;

	while (this->GetIsTracing()) {

		// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
		// Check if the current thread should be closed
		if (current.ttl > this->GetMax()) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&current.address);
		const auto dwReplyCount = IcmpSendEcho(current.icmpHandle.Get(), addr->sin_addr.S_un.S_addr, achReqData.data(), nDataLen, &stIPInfo, achRepData.data(), gsl::narrow_cast<DWORD>(achRepData.size()), ECHO_REPLY_TIMEOUT);

		this->AddXmit(current.ttl - 1);
		if (dwReplyCount != 0) {
			PICMPECHO icmp_echo_reply = reinterpret_cast<PICMPECHO>(achRepData.data());
			TRACE_MSG(L"TTL "sv << current.ttl << L" reply TTL "sv << icmp_echo_reply->Options.Ttl << L" Status "sv << icmp_echo_reply->Status << L" Reply count "sv << dwReplyCount);

			switch (icmp_echo_reply->Status) {
			case IP_SUCCESS:
			case IP_TTL_EXPIRED_TRANSIT:
				this->SetLast(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetBest(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetWorst(current.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->AddReturned(current.ttl - 1);
				{
					sockaddr_in naddr = {};
					naddr.sin_family = AF_INET;
					naddr.sin_addr.S_un.S_addr = icmp_echo_reply->Address;
					this->SetAddr(current.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
				}
				break;
			default:
				this->handleDefault(current, icmp_echo_reply->Status);
				break;
			}
			this->sleepTilInterval(icmp_echo_reply->RoundTripTime);
		}

	} /* end ping loop */
}

void WinMTRNet::DoTrace(sockaddr& address)
{
	std::vector<std::thread> threads;
	threads.reserve(MAX_HOPS);
	tracing = true;

	ResetHops();
	memcpy(&last_remote_addr, &address, getAddressSize(address));
	// one thread per TTL value
	for (int i = 0; i < MAX_HOPS; i++) {
		trace_thread current(address.sa_family, this, i + 1);
		memcpy(&current.address, &address, getAddressSize(address));
		threads.emplace_back([current = std::move(current)]() mutable {
			using namespace std::chrono_literals;
			using namespace std::string_literals;
			WinMTRNet* wmtrnet = current.winmtr;
			TRACE_MSG(L"Threaad with TTL=" << current.ttl << L" started.");

			if (current.address.ss_family == AF_INET) {
				wmtrnet->handleICMPv4(current);
			}
			else if (current.address.ss_family == AF_INET6) {
				wmtrnet->handleICMPv6(current);
			}

			TRACE_MSG(L"Thread with TTL=" << current.ttl << L" stopped.");
		});
	}

	for (auto& thread : threads) {
		thread.join();
	}
	TRACE_MSG(L"Tracing Ended");
}

void WinMTRNet::StopTrace() noexcept
{
	tracing = false;
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
		DWORD addrstrsize = gsl::narrow_cast<DWORD>(out.size());
		if (auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), gsl::narrow_cast<DWORD>(addrlen), nullptr, out.data(), &addrstrsize); !result) {
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
	for (int i = 0; i < MAX_HOPS; i++) {
		if (memcmp(&host[i].addr, &last_remote_addr, sizeof(SOCKADDR_STORAGE)) == 0) {
			max = i + 1;
			break;
		}
	}

	// second match:  traced address doesn't responds on ping requests
	if (max == MAX_HOPS) {
		while ((max > 1) && (memcmp(&host[max - 1].addr, &host[max - 2].addr, sizeof(SOCKADDR_STORAGE)) == 0 && isValidAddress(host[max - 1].addr))) max--;
	}
	return max;
}

double WinMTRNet::GetInterval() const noexcept
{
	return this->wmtrdlg->interval;
}


int WinMTRNet::GetPingSize() const noexcept
{
	return this->wmtrdlg->pingsize;
}

void WinMTRNet::SetAddr(int at, sockaddr& addr)
{
	std::unique_lock lock(ghMutex);
	if (!isValidAddress(host[at].addr) && isValidAddress(addr)) {
		//TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
		memcpy(&host[at].addr, &addr, getAddressSize(addr));
		struct dns_resolver_thread {
			int			index;
			WinMTRNet* winmtr;
		} dnt{ .index = at, .winmtr = this };

		if (wmtrdlg->useDNS) {
			Concurrency::create_task([dnt = std::move(dnt)] {
					TRACE_MSG(L"DNS resolver thread started.");
					WinMTRNet * wn = dnt.winmtr;

					wchar_t buf[NI_MAXHOST];
					sockaddr_storage addr = wn->GetAddr(dnt.index);

					if (const auto nresult = GetNameInfoW(
						reinterpret_cast<sockaddr*>(&addr)
						, gsl::narrow_cast<socklen_t>(getAddressSize(addr))
						, buf
						, gsl::narrow_cast<DWORD>(std::size(buf))
						, nullptr
						, 0
						, 0);
						// zero on success
						!nresult) {
						wn->SetName(dnt.index, buf);
					}
					else {
						std::wstring out;
						out.resize(40);
						auto addrlen = gsl::narrow_cast<DWORD>(getAddressSize(addr));
						DWORD addrstrsize = gsl::narrow_cast<DWORD>(out.size());
						if (const auto result = WSAAddressToStringW(
							reinterpret_cast<LPSOCKADDR>(&addr)
							,addrlen
							,nullptr
							,out.data()
							,&addrstrsize);
							// zero on success
							!result) {
							out.resize(addrstrsize - 1);
						}
						wn->SetName(dnt.index, std::move(out));
					}

					TRACE_MSG(L"DNS resolver thread stopped.");
				});
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
	if (host[at].best > current || host[at].xmit == 1) {
		host[at].best = current;
	};
	if (host[at].worst < current) {
		host[at].worst = current;
	}
}

void WinMTRNet::SetWorst(int at, int current)
{
	UNREFERENCED_PARAMETER(at);
	UNREFERENCED_PARAMETER(current);
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



