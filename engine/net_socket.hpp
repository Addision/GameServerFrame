#pragma once
#include "net_defines.hpp"
#include <string>

namespace Engine
{
	enum class SockType { tcp, udp };
	class ISocket
	{
		typedef union address
		{
			struct sockaddr sa;
			struct sockaddr_in sa_in;
			struct sockaddr_in6 sa_in6;
			//struct sockaddr_storage sa_stor;
		}address_t;
	public:
		virtual fd_t create_fd() = 0;
		void create_socket();
		void close();
		void close_fd(fd_t& fd);
		// server
		bool bind(const char* host, std::uint32_t port);
		fd_t accept(struct sockaddr* addr, socklen_t len);
		// client
		bool connect(const char* host, std::uint32_t port, std::uint32_t timeout_msecs);

		bool is_ipv6();
		void set_fd(const fd_t& fd, struct sockaddr* addr);
	public:
		std::string get_host() { return m_host; }
		std::uint32_t get_port() { return m_port; }
		fd_t get_fd() { return m_fd; } 

		void set_nonblock();
		void set_reuseaddr(bool optval);
		void set_nodelay(bool optval);
		void set_keepalive(bool on, std::uint32_t idle, std::uint32_t interval, std::uint32_t cnt);
		void set_buffersize(std::uint32_t send_size, std::uint32_t recv_size);
		void set_broadcast(bool optval);
		template<class T>
		bool set_opt(std::int32_t lv, std::int32_t flag, T val)
		{
			return setsockopt(m_fd, lv, flag, (const char*)&val, sizeof(val));
		}
		virtual void set_opts() = 0;
		bool set_addr(address_t& address, socklen_t& len);
	protected:
		fd_t m_fd;
		std::string m_host;
		std::uint32_t m_port;
	};

	class Socket : public ISocket
	{
	public:
		fd_t create_fd();
		void set_opts();
	private:
	};

	class UdpSocket : public ISocket
	{
	public:
		fd_t create_fd();
		void set_opts();
		int send(const char* data, size_t len);
		int recv(char* buf, size_t len);
	private:
		struct sockaddr m_addr; // 需要通信地址
	};
}
