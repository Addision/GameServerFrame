#pragma once
#include "engine_defines.hpp"

#ifdef _WIN
#include <Winsock2.h>
#include <Windows.h>
#include <Mswsock.h>
#include <Ws2tcpip.h>
#include <Mstcpip.h>
#undef min
#undef max
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#endif

#ifdef _WIN
	using fd_t = SOCKET;
#else
	using fd_t = int;
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#define SD_BOTH SHUT_RDWR
	#define ioctlsocket ioctl
	#define WSAETIMEDOUT ETIMEDOUT
	#define WSAEWOULDBLOCK EWOULDBLOCK 
	#define WSAEALREADY EALREADY
	#define WSAEINPROGRESS EINPROGRESS
	#define WSAEINTR EINTR
	#define WSAECONNREFUSED ECONNREFUSED

	int WSAGetLastError() { return errno; }
	int closesocket(fd_t& fd) { return close(fd); }

	struct OVERLAPPED
	{
		std::atomic_bool m_inuse;
		std::atomic_bool m_need_send;
	};

	struct WSABUF
	{
		unsigned long len;
		char* buf;
	};
#endif

#define DEFAULT_SOCKET_READ_LEN 1024
#define SOCKET_BUFFER_SIZE 40*1024
#define CONNECT_TIME_OUT 3000
#define SESSION_INIT_SIZE 2000
#define SESSION_GROW_SIZE 100
#define CONNECT_TIMEOUT 3000
#define LOOP_TIMEOUT 5

#define SOCKET_ERR_IS_EAGAIN(e) ((e) == WSAEWOULDBLOCK || (e) == EAGAIN)
#define SOCKET_ERR_RW_RETRIABLE(e) ((e) == WSAEINTR || SOCKET_ERR_IS_EAGAIN(e))
#define SOCKET_CONN_CLOSE(e) ((e) == ECONNABORTED || (e) == ECONNRESET)
#define SOCKET_ERR_CONNECT_REFUSED(e) ((e) == WSAECONNREFUSED)

enum EventType
{
	EV_NONE = 0,
	EV_TIMEOUT = 1,
	EV_READ = 2,
	EV_WRITE = 4,
	EV_CLOSED = 8,
	EV_ACCEPT = 16,
};

enum NetEventType
{
	NET_EV_EOF = 0x10,
	NET_EV_ERROR = 0x20,	
	NET_EV_TIMEOUT = 0x40,
	NET_EV_CONNECTED = 0x80,
};

enum class SessionCloseType
{
	none = 0,
	service_stop,
	connect_timeout,
	connect_peer_close,
	send_buffer_overflow,
	recv_buffer_overflow,
	pack_error,
	unpack_error,
	handle_error,
	null_handle,
	handle_unknow_err,
	recv_unknow_err,
	send_unknow_err,
};

struct IO_DATA
{
	OVERLAPPED m_overlapped;
	WSABUF m_buffer; // {len, buf}
	EventType m_event;
	void* m_owner{nullptr};
	fd_t m_fd{INVALID_SOCKET};
};

struct AcceptData : public IO_DATA
{
	char m_buff[sizeof(sockaddr) * 2 + 50];
};

#ifdef _WIN
#define _IOCP
#else
#define _EPOLL
#endif

