//*****************************************************************************
// FILE:            WinMTRNet.cpp
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRNet.h"
#include "WinMTRDialog.h"
#include <wrl/wrappers/corewrappers.h>
#include "resource.h"


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
#define PIO_APC_ROUTINE_DEFINED
#include <iphlpapi.h>
#include <icmpapi.h>

namespace {
	
constexpr auto ECHO_REPLY_TIMEOUT = 5000;

	struct ICMPHandleTraits {
		using Type = HANDLE;
		inline static bool Close(_In_ Type h) noexcept
		{
			return ::IcmpCloseHandle(h) != FALSE;
		}

		[[nodiscard]]
		inline static constexpr Type GetInvalidValue() noexcept
		{
			return INVALID_HANDLE_VALUE;
		}
	};

	using IcmpHandle = wrl::Wrappers::HandleT<ICMPHandleTraits>;

	struct awaitable_base {
#ifdef __has_include                           // Check if __has_include is present
#  if __has_include(<coroutine>)                // Check for a standard library
		using coro_handle = std::coroutine_handle<>;
#  else
		using coro_handle = std::experimental::coroutine_handle<>;
#  endif
#endif
		awaitable_base(HANDLE icmpHandle, UCHAR ttl, std::span<std::byte> requestData, std::span<std::byte> replyData) noexcept
			:m_reqData(requestData)
			, m_replyData(replyData)
			, m_handle(icmpHandle)
			, m_ttl(ttl)
		{

		}

		bool await_ready() const noexcept
		{
			return false;
		}
	protected:
		static void NTAPI callback(IN PVOID ApcContext,
			IN PIO_STATUS_BLOCK IoStatusBlock,
			IN ULONG Reserved) noexcept {
			UNREFERENCED_PARAMETER(Reserved);
			auto context = static_cast<awaitable_base*>(ApcContext);
			context->m_replysize = gsl::narrow_cast<DWORD>(IoStatusBlock->Information);
			concurrency::create_task([=] {
				context->m_resume();
			}, context->m_context);
		}

		concurrency::task_continuation_context m_context = concurrency::task_continuation_context::get_current_winrt_context();
		coro_handle m_resume{ nullptr };
		std::span<std::byte> m_reqData;
		std::span<std::byte> m_replyData;
		HANDLE m_handle;
		DWORD m_replysize = 0;
		UCHAR m_ttl;
	};

	struct icmp_ping4 final : awaitable_base
	{
		[[nodiscard]]
		static constexpr auto reply_reply_buffer_size(unsigned requestSize) noexcept {
			return sizeof(ICMP_ECHO_REPLY) + 8 + sizeof(IO_STATUS_BLOCK) + requestSize;
		}

		icmp_ping4(HANDLE icmpHandle, IPAddr addr, UCHAR ttl, std::span<std::byte> requestData, std::span<std::byte> replyData) noexcept
			:awaitable_base(icmpHandle, ttl, requestData, replyData)
			, m_addr(addr)
		{
		}

		void await_suspend(awaitable_base::coro_handle resume_handle)
		{
			m_resume = resume_handle;
			IP_OPTION_INFORMATION	stIPInfo = {
				.Ttl = m_ttl,
				.Flags = IP_FLAG_DF
			};

			// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
			// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
			// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
			// A resolution would be:
			// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
			// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
			// for these servers we'll have 100% loss
			const auto io_res = IcmpSendEcho2(
				m_handle
				, nullptr
				, &awaitable_base::callback
				, this
				, m_addr
				, m_reqData.data()
				, gsl::narrow<WORD>(m_reqData.size())
				, &stIPInfo
				, m_replyData.data()
				, gsl::narrow_cast<DWORD>(m_replyData.size())
				, ECHO_REPLY_TIMEOUT);

			if (const auto err = GetLastError(); err != ERROR_IO_PENDING) {
				throw std::system_error(err, std::system_category());
			}
		}

		auto await_resume() const noexcept
		{
			return IcmpParseReplies(m_replyData.data(), m_replysize);
		}

	private:
		IPAddr m_addr;
	};

	struct icmp_ping6 final : awaitable_base
	{
		[[nodiscard]]
		static constexpr auto reply_reply_buffer_size(unsigned requestSize) noexcept {
			return sizeof(ICMPV6_ECHO_REPLY) + 8 + sizeof(IO_STATUS_BLOCK) + requestSize;
		}

		icmp_ping6(HANDLE icmpHandle, sockaddr_in6* addr, UCHAR ttl, std::span<std::byte> requestData, std::span<std::byte> replyData) noexcept
			:awaitable_base(icmpHandle, ttl, requestData, replyData)
			, m_addr(addr)
		{
		}

		void await_suspend(awaitable_base::coro_handle resume_handle)
		{
			m_resume = resume_handle;
			IP_OPTION_INFORMATION	stIPInfo = {
				.Ttl = m_ttl,
				.Flags = IP_FLAG_DF
			};
			SOCKADDR_IN6 local = { .sin6_family = AF_INET6, .sin6_addr = in6addr_any };

			// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
			// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
			// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
			// A resolution would be:
			// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
			// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
			// for these servers we'll have 100% loss
			const auto io_res = Icmp6SendEcho2(
				m_handle
				, nullptr
				, &awaitable_base::callback
				, this
				, &local
				, m_addr
				, m_reqData.data()
				, gsl::narrow<WORD>(m_reqData.size())
				, &stIPInfo
				, m_replyData.data()
				, gsl::narrow_cast<DWORD>(m_replyData.size())
				, ECHO_REPLY_TIMEOUT);

			if (const auto err = GetLastError(); err != ERROR_IO_PENDING) {
				throw std::system_error(err, std::system_category());
			}
		}

		auto await_resume() const noexcept
		{
			return Icmp6ParseReplies(m_replyData.data(), m_replysize);
		}

	private:
		sockaddr_in6* m_addr;
	};

	[[nodiscard]]
	inline bool operator==(const SOCKADDR_STORAGE& lhs, const SOCKADDR_STORAGE& rhs) noexcept {
		return memcmp(&lhs, &rhs, sizeof(SOCKADDR_STORAGE)) == 0;
	}
}

using namespace std::chrono_literals;
struct trace_thread {
	trace_thread(const trace_thread&) = delete;
	trace_thread& operator=(const trace_thread&) = delete;
	trace_thread(trace_thread&&) = default;
	trace_thread& operator=(trace_thread&&) = default;

	trace_thread(ADDRESS_FAMILY af, WinMTRNet* winmtr, UCHAR ttl) noexcept
		:
		address(),
		winmtr(winmtr),
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


WinMTRNet::WinMTRNet(const WinMTRDialog* wp)
	:host(),
	last_remote_addr(),
	wmtrdlg(wp),
	wsaHelper(MAKEWORD(2, 2)),
	tracing(false) {

	if (!wsaHelper) [[unlikely]] {
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return;
	}
}

WinMTRNet::~WinMTRNet() noexcept
{}

void WinMTRNet::ResetHops() noexcept
{
	for (auto& h : this->host) {
		h = s_nethost();
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

concurrency::task<void> WinMTRNet::sleepTilInterval(ULONG roundTripTime) {
	using namespace winrt;
	using namespace std::chrono_literals;
	const auto intervalInSec = this->wmtrdlg->getInterval() * 1s;
	const auto roundTripDuration = std::chrono::milliseconds(roundTripTime);
	if (intervalInSec > roundTripDuration) {
		const auto sleepTime = intervalInSec - roundTripDuration;
		co_await std::chrono::duration_cast<winrt::Windows::Foundation::TimeSpan>(sleepTime);
	}
}


winrt::Windows::Foundation::IAsyncAction WinMTRNet::handleICMPv6(trace_thread current) {
	trace_thread mine = std::move(current);
	co_await winrt::resume_background();
	using namespace std::string_view_literals;
	const auto				nDataLen = this->wmtrdlg->getPingSize();
	std::vector<std::byte>	achReqData(nDataLen, static_cast<std::byte>(32)); //whitespaces
	std::vector<std::byte> achRepData(icmp_ping6::reply_reply_buffer_size(nDataLen));

	SOCKADDR_IN6* addr = reinterpret_cast<SOCKADDR_IN6*>(&mine.address);

	
	auto cancellation = co_await winrt::get_cancellation_token();

	while (this->tracing) {
		if (cancellation()) [[unlikely]]
		{
			co_return;
		}
		// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
		// Check if the current thread should be closed
		if (mine.ttl > this->GetMax()) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss

		const auto dwReplyCount = co_await icmp_ping6(mine.icmpHandle.Get(), addr, mine.ttl, achReqData, achRepData);
		this->AddXmit(mine.ttl - 1);
		if(dwReplyCount){

			PICMPV6_ECHO_REPLY icmp_echo_reply = reinterpret_cast<PICMPV6_ECHO_REPLY>(achRepData.data());
			TRACE_MSG(L"TTL "sv << mine.ttl << L" Status "sv << icmp_echo_reply->Status << L" Reply count "sv << dwReplyCount)
			switch (icmp_echo_reply->Status) {
			case IP_SUCCESS:
			[[likely]] case IP_TTL_EXPIRED_TRANSIT:
				this->SetLast(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetBest(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetWorst(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->AddReturned(mine.ttl - 1);
				{
					sockaddr_in6 naddr = { .sin6_family = AF_INET6 };
					memcpy(&naddr.sin6_addr, icmp_echo_reply->Address.sin6_addr, sizeof(naddr.sin6_addr));
					this->SetAddr(mine.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
				}
				break;
			default:
				this->handleDefault(mine, icmp_echo_reply->Status);
				break;
			}
			co_await this->sleepTilInterval(icmp_echo_reply->RoundTripTime);
		}
	}
}

winrt::Windows::Foundation::IAsyncAction WinMTRNet::handleICMPv4(trace_thread current) {
	trace_thread mine = std::move(current);
	co_await winrt::resume_background();
	using namespace std::string_view_literals;
	const auto				nDataLen = this->wmtrdlg->getPingSize();
	std::vector<std::byte>	achReqData(nDataLen, static_cast<std::byte>(32)); //whitespaces
	std::vector<std::byte> achRepData(icmp_ping4::reply_reply_buffer_size(nDataLen));

	auto cancellation = co_await winrt::get_cancellation_token();
	
	while (this->tracing) {
		if (cancellation()) [[unlikely]]
		{
			co_return;
		}
		// For some strange reason, ICMP API is not filling the TTL for icmp echo reply
		// Check if the current thread should be closed
		if (mine.ttl > this->GetMax()) break;

		// NOTE: some servers does not respond back everytime, if TTL expires in transit; e.g. :
		// ping -n 20 -w 5000 -l 64 -i 7 www.chinapost.com.tw  -> less that half of the replies are coming back from 219.80.240.93
		// but if we are pinging ping -n 20 -w 5000 -l 64 219.80.240.93  we have 0% loss
		// A resolution would be:
		// - as soon as we get a hop, we start pinging directly that hop, with a greater TTL
		// - a drawback would be that, some servers are configured to reply for TTL transit expire, but not to ping requests, so,
		// for these servers we'll have 100% loss
		sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&mine.address);
		
		const auto dwReplyCount = co_await icmp_ping4(mine.icmpHandle.Get(), addr->sin_addr.S_un.S_addr, mine.ttl, achReqData, achRepData);
		this->AddXmit(mine.ttl - 1);
		if(dwReplyCount){
			PICMP_ECHO_REPLY icmp_echo_reply = reinterpret_cast<PICMP_ECHO_REPLY>(achRepData.data());
			TRACE_MSG(L"TTL "sv << mine.ttl << L" reply TTL "sv << icmp_echo_reply->Options.Ttl << L" Status "sv << icmp_echo_reply->Status << L" Reply count "sv << dwReplyCount);

			switch (icmp_echo_reply->Status) {
			case IP_SUCCESS:
			[[likely]] case IP_TTL_EXPIRED_TRANSIT:
				this->SetLast(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetBest(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->SetWorst(mine.ttl - 1, icmp_echo_reply->RoundTripTime);
				this->AddReturned(mine.ttl - 1);
				{
					sockaddr_in naddr = {};
					naddr.sin_family = AF_INET;
					naddr.sin_addr.S_un.S_addr = icmp_echo_reply->Address;
					this->SetAddr(mine.ttl - 1, *reinterpret_cast<sockaddr*>(&naddr));
				}
				break;
			default:
				this->handleDefault(mine, icmp_echo_reply->Status);
				break;
			}
			co_await this->sleepTilInterval(icmp_echo_reply->RoundTripTime);
		}

	} /* end ping loop */
}


winrt::Windows::Foundation::IAsyncAction WinMTRNet::DoTrace(sockaddr& address)
{
	auto cancellation = co_await winrt::get_cancellation_token();
	cancellation.enable_propagation();
	tracing = true;
	ResetHops();
	memcpy(&last_remote_addr, &address, getAddressSize(address));
	
	auto threadMaker = [&address, this](UCHAR i) {
		trace_thread current(address.sa_family, this, i + 1);
		using namespace std::string_view_literals;
		TRACE_MSG(L"Thread with TTL="sv << current.ttl << L" started."sv);
		memcpy(&current.address, &address, getAddressSize(address));
		if (current.address.ss_family == AF_INET) {
			return this->handleICMPv4(std::move(current));
		}
		else if (current.address.ss_family == AF_INET6) {
			return this->handleICMPv6(std::move(current));
		}
		winrt::throw_hresult(HRESULT_FROM_WIN32(WSAEOPNOTSUPP));
	};

	cancellation.callback([this] () noexcept {
		this->tracing = false;
		TRACE_MSG(L"Cancellation");
	});
	//// one thread per TTL value
	co_await std::invoke([]<UCHAR ...threads>(std::integer_sequence<UCHAR, threads...>, auto threadMaker) {
		return winrt::when_all(std::invoke(threadMaker, threads)...);
	}, std::make_integer_sequence<UCHAR, MAX_HOPS>(), threadMaker);
	TRACE_MSG(L"Tracing Ended");
}

sockaddr_storage WinMTRNet::GetAddr(int at) const
{
	std::unique_lock lock(ghMutex);
	return host[at].addr;
}

std::wstring WinMTRNet::GetName(int at)
{
	std::unique_lock lock(ghMutex);
	return host[at].getName();
}

int WinMTRNet::GetBest(int at) const
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].best;
	return ret;
}

int WinMTRNet::GetWorst(int at) const
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].worst;
	return ret;
}

int WinMTRNet::GetAvg(int at) const
{
	std::unique_lock lock(ghMutex);
	return host[at].getAvg();
}

int WinMTRNet::GetPercent(int at) const
{
	std::unique_lock lock(ghMutex);
	return host[at].getPercent();
}

int WinMTRNet::GetLast(int at) const
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].last;
	return ret;
}

int WinMTRNet::GetReturned(int at) const
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].returned;
	return ret;
}

int WinMTRNet::GetXmit(int at) const
{
	std::unique_lock lock(ghMutex);
	int ret = host[at].xmit;
	return ret;
}

int WinMTRNet::GetMax() const
{
	std::unique_lock lock(ghMutex);
	int max = MAX_HOPS;

	// first match: traced address responds on ping requests, and the address is in the hosts list
	for (int i = 1; const auto & h : host) {
		if (h.addr == last_remote_addr) {
			max = i;
			break;
		}
		++i;
	}

	// second match:  traced address doesn't responds on ping requests
	if (max == MAX_HOPS) {
		while ((max > 1) && (host[max - 1].addr == host[max - 2].addr && isValidAddress(host[max - 1].addr))) max--;
	}
	return max;
}

std::vector<s_nethost> WinMTRNet::getCurrentState() const
{
	std::unique_lock lock(ghMutex);
	auto max = GetMax();
	auto end = std::cbegin(host);
	std::advance(end, max);
	return std::vector<s_nethost>(std::cbegin(host), end);
}

void WinMTRNet::SetAddr(int at, sockaddr& addr)
{
	std::unique_lock lock(ghMutex);
	if (!isValidAddress(host[at].addr) && isValidAddress(addr)) {
		//TRACE_MSG(L"Start DnsResolverThread for new address " << addr << L". Old addr value was " << host[at].addr);
		memcpy(&host[at].addr, &addr, getAddressSize(addr));

		if (wmtrdlg->getUseDNS()) {
			Concurrency::create_task([at, sharedThis = shared_from_this()] {
					TRACE_MSG(L"DNS resolver thread started.");

					wchar_t buf[NI_MAXHOST];
					sockaddr_storage addr = sharedThis->GetAddr(at);

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
						sharedThis->SetName(at, buf);
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
						sharedThis->SetName(at, std::move(out));
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

std::wstring s_nethost::getName() const
{
	if (name.empty()) {
		SOCKADDR_STORAGE laddr = addr;
		if (!isValidAddress(addr)) {
			return {};
		}
		std::wstring out;
		out.resize(40);
		auto addrlen = getAddressSize(addr);
		DWORD addrstrsize = gsl::narrow_cast<DWORD>(out.size());
		if (auto result = WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&laddr), gsl::narrow_cast<DWORD>(addrlen), nullptr, out.data(), &addrstrsize); !result) {
			out.resize(addrstrsize - 1);
		}

		return out;
	}
	return name;
}
