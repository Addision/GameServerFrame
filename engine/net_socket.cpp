#include "net_socket.hpp"
#include "net_base.hpp"

#define SOCKET_LOG
#ifdef SOCKET_LOG
#define SOCKET_DEBUG(fmt,...) ENGINE_LOG(fmt,##__VA_ARGS__)
#else
#define SOCKET_DEBUG(fmt,...)
#endif

namespace Engine
{
	void ISocket::create_socket()
	{
		m_fd = create_fd();
	}
	bool ISocket::is_ipv6()
	{
		return m_host.find(':') != std::string::npos;
	}

	void ISocket::set_fd(const fd_t& fd, struct sockaddr* addr)
	{
		m_fd = fd;
		if (addr == nullptr) return;
		//! ipv6
		if (addr->sa_family == AF_INET6) {
			struct sockaddr_in6* addr6 = reinterpret_cast<struct sockaddr_in6*>(addr);
			char buf[INET6_ADDRSTRLEN] = {};
			m_host = std::string("[") + ::inet_ntop(addr->sa_family, &addr6->sin6_addr, buf, INET6_ADDRSTRLEN) + "]";
			m_port = ntohs(addr6->sin6_port);
		}
		//! ipv4
		else {
			struct sockaddr_in* addr4 = reinterpret_cast<struct sockaddr_in*>(addr);
			char buf[INET_ADDRSTRLEN] = {};
			m_host = ::inet_ntop(addr->sa_family, &addr4->sin_addr, buf, INET_ADDRSTRLEN);
			m_port = ntohs(addr4->sin_port);
		}
		SOCKET_DEBUG("connected from ip:%s port%d", m_host.c_str(), m_port);
	}

	void ISocket::close()
	{
		close_fd(m_fd);
	}
	void ISocket::close_fd(fd_t& fd)
	{
		if (fd == INVALID_SOCKET) { return; }
		shutdown(fd, SD_BOTH);
		closesocket(fd);
		fd = INVALID_SOCKET;
	}

	bool ISocket::bind(const char* host, std::uint32_t port)
	{
		m_host = host;
		m_port = port;
		create_socket();
		address_t address;
		memset(&address, 0, sizeof(address));
		socklen_t socklen;
		set_addr(address, socklen);
		if (::bind(m_fd, &address.sa, socklen) < 0 )
		{
			return false;
		}
		if (::listen(m_fd, SOMAXCONN) < 0)
		{
			return false;
		}
		return true;
	}

	fd_t ISocket::accept(struct sockaddr* addr, socklen_t len)
	{
		fd_t connfd = INVALID_SOCKET;
		connfd = ::accept(m_fd, addr, &len);
		return connfd;
	}

	bool ISocket::connect(const char* host, std::uint32_t port, std::uint32_t timeout_msecs)
	{
		m_host = host;
		m_port = port;
		create_socket();
		address_t address;
		memset(&address, 0, sizeof(address));
		socklen_t socklen;
		set_addr(address, socklen);
		if (timeout_msecs > 0)
		{
			set_nonblock();
		}
		int ret = ::connect(m_fd, &address.sa, socklen);
		if (ret == 0) 
		{
			set_nonblock();
			return true;
		}
		if (ret == SOCKET_ERROR)
		{
			int err_code = NetBase::get_socket_err(m_fd);
			if (err_code == WSAETIMEDOUT)
			{
				close();
				return false;
			}
			if (err_code != WSAEWOULDBLOCK && err_code != WSAEALREADY && err_code != WSAEINPROGRESS && err_code != 0)
			{
				close();
				return false;
			}
		}
		if(timeout_msecs>0)
		{
			timeval tv;
			tv.tv_sec = (timeout_msecs / 1000);
			tv.tv_usec = ((timeout_msecs - (tv.tv_sec * 1000)) * 1000);

			fd_set set;
			FD_ZERO(&set);
			FD_SET(m_fd, &set);
			//! 1 means we are connected.
			//! 0 means a timeout.
			do {
				int ret = select(static_cast<int>(m_fd) + 1, NULL, &set, NULL, &tv);
				if (ret < 0) {
					int error_code = WSAGetLastError();
					if (error_code == WSAEINTR) 
					{ continue; }
					close();
					return false;
				}
				if (ret == 0) {
					close();
					return false;
				}
				//! Make sure there are no async connection errors
				int err = 0;
				socklen_t len = sizeof(err);
				if(NetBase::get_socket_err(m_fd) == SOCKET_ERROR)
				if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len) == -1) {
					close();
					return false;
				}
				break;
			} while (true);
		}
		set_nonblock();
		return true;
	}
	bool ISocket::set_addr(address_t& address, socklen_t& addr_len)
	{
		memset(&address, 0, sizeof(address));
		if (is_ipv6())
		{
			struct sockaddr_in6* addr = &address.sa_in6;
			if (::inet_pton(AF_INET6, m_host.c_str(), &addr->sin6_addr) < 0)
			{
				return false;
			}
			addr->sin6_port = htons(m_port);
			addr->sin6_family = AF_INET6;
			addr_len = sizeof(*addr);
		}
		else
		{
			struct addrinfo* result = nullptr;
			if (getaddrinfo(m_host.c_str(), nullptr, nullptr, &result) != 0)
			{
				return false;
			}
			struct sockaddr_in* addr = &address.sa_in;
			addr->sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
			addr->sin_port = htons(m_port);
			addr->sin_family = AF_INET;
			addr_len = sizeof(*addr);
			freeaddrinfo(result);
		}
		return true;
	}

	void ISocket::set_nonblock()
	{
		unsigned long ul = 1;
		ioctlsocket(m_fd, FIONBIO, &ul);
	}
	void ISocket::set_reuseaddr(bool optval)
	{
		set_opt(SOL_SOCKET, SO_REUSEADDR, optval);
	}
	void ISocket::set_nodelay(bool optval)
	{
		set_opt(IPPROTO_TCP, TCP_NODELAY, optval);
	}
	void ISocket::set_keepalive(bool on, std::uint32_t idle, std::uint32_t interval, std::uint32_t cnt)
	{
		set_opt(SOL_SOCKET, SO_KEEPALIVE, on);
		if (!on)
			return;
#ifdef _WIN
		struct tcp_keepalive in_keep_alive = { 0 };
		struct tcp_keepalive out_keep_alive = { 0 };
		DWORD ul_in_len = sizeof(in_keep_alive);
		DWORD ul_out_len = sizeof(out_keep_alive);
		DWORD ul_bytes_return = 0;
		in_keep_alive.onoff = 1;
		in_keep_alive.keepaliveinterval = interval;
		in_keep_alive.keepalivetime = idle;
		WSAIoctl(m_fd, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
			(LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
#else
		set_opt(IPPROTO_TCP, TCP_KEEPIDLE, idle / 1000);
		set_opt(IPPROTO_TCP, TCP_KEEPINTVL, interval / 1000);
		set_opt(IPPROTO_TCP, TCP_KEEPCNT, cnt)
#endif
	}
	void ISocket::set_buffersize(std::uint32_t send_size, std::uint32_t recv_size)
	{
		set_opt(SOL_SOCKET, SO_SNDBUF, send_size);
		set_opt(SOL_SOCKET, SO_RCVBUF, recv_size);
	}
	void ISocket::set_broadcast(bool optval)
	{
		set_opt(SOL_SOCKET, SO_BROADCAST, optval);
	}
////////////////////////////////////////////////////////////////////////////////////
	fd_t Socket::create_fd()
	{
		int family = is_ipv6() ? AF_INET6 : AF_INET;
#if defined(_IOCP) && defined(_WIN)
		return ::WSASocket(family, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
		return ::socket(family, SOCK_STREAM, IPPROTO_TCP);
#endif
	}
	void Socket::set_opts()
	{
		set_reuseaddr(true);
		set_nodelay(true);
		set_buffersize(SOCKET_BUFFER_SIZE, SOCKET_BUFFER_SIZE);
		set_nonblock();
		set_keepalive(true, 1000, 1000, 15);
	}

////////////////////////////////////////////////////////////////////////////////////
	fd_t UdpSocket::create_fd()
	{
		int family = is_ipv6() ? AF_INET6 : AF_INET;
#if defined(_IOCP) && defined(_WIN)
		return ::WSASocket(family, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
		return ::socket(family, SOCK_DGRAM, IPPROTO_UDP);
#endif
	}

	void UdpSocket::set_opts()
	{

	}

	int UdpSocket::send(const char* data, size_t len)
	{
		socklen_t addr_len = sizeof(m_addr);
		return sendto(m_fd, data, len, 0, &m_addr, addr_len);
	}

	int UdpSocket::recv(char* buf, size_t len)
	{
		socklen_t addr_len = sizeof(m_addr);
		return ::recvfrom(m_fd, buf, len, 0, &m_addr, &addr_len);
	}

}
