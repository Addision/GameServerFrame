#ifdef _WIN
#include "io_iocp.hpp"
#include "com_util.hpp"
#include "net_socket.hpp"
#include "net_service_server.hpp"
#include "net_service_client.hpp"
#include "net_session.hpp"
#include <functional>

#pragma comment(lib,"Kernel32.lib") // IOCP需要用到的动态链接库
#pragma comment(lib, "Mswsock.lib")

#define IOCP_LOG
#ifdef IOCP_LOG
#define IOCP_DEBUG(fmt,...) NET_DEBUG(fmt,##__VA_ARGS__)
#else
#define IOCP_DEBUG(fmt,...)
#endif

namespace Engine
{
	IocpService::IocpService(void)
	{
		m_state = state::net_none;
	}
	IocpService::~IocpService(void)
	{
		stop();
	}
	void IocpService::creat_iocp()
	{
		if (m_hiocp == nullptr)
			return;
		m_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, n_threads);
	}
	void IocpService::start()
	{
		IocpService::service_start();
		creat_iocp();
		for (std::uint32_t i = 0; i < n_threads; ++i)
		{
			m_threads.emplace_back(std::thread(std::bind(&IocpService::Loop, this, LOOP_TIMEOUT)));
		}
	}

	void IocpService::stop()
	{
		state st = state::net_running;
		if (!m_state.compare_exchange_strong(st, state::net_stop))
		{
			if (st == state::net_start)
			{
				LogUtil::fatal(EngineErrs::E_NET, "stop io_service_iocp on starting!");
			}
			IOCP_DEBUG("already stopping!");
			return;
		}

		IOCP_DEBUG("iocp stopping!");

		for (size_t i = 0; i < m_threads.size(); ++i)
			PostQueuedCompletionStatus(m_hiocp, -1, NULL, NULL);

		for (std::thread& it : m_threads)
			it.join();

		close();
		m_state = state::net_none;
		IOCP_DEBUG("iocp stopped!");
	}
	bool IocpService::bind_socket(fd_t& fd)
	{
		if (CreateIoCompletionPort((HANDLE)fd, m_hiocp, fd, 0) != m_hiocp) // 第三个值是completkey
		{
			return false;
		}
		return true;
	}
	void IocpService::bind_server(ServiceServer* server)
	{
		fd_t fd = server->get_fd();

		DWORD dwbytes = 0;
		GUID guidAcceptEx = WSAID_ACCEPTEX;

		if (0 != WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(guidAcceptEx),
			&m_accept, sizeof(m_accept),
			&dwbytes, NULL, NULL))
		{
			std::cout << "get AcceptEx failure!" << std::endl;
		}

		GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		if (0 != WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidGetAcceptExSockaddrs,
			sizeof(guidGetAcceptExSockaddrs),
			&m_accept_addrs,
			sizeof(m_accept_addrs),
			&dwbytes, NULL, NULL))
		{
			std::cout << "get AcceptEx failure!" << std::endl;
		}

		if (bind_socket(fd))
		{
			while (AcceptData* data = server->get_accept_data())
			{
				data->m_event = EventType::EV_ACCEPT;
				data->m_owner = server;
				data->m_buffer.buf = data->m_buff;
				data->m_buffer.len = sizeof(data->m_buff);

				post_accept(server, data);
			}
			return;
		}
		LogUtil::fatal(EngineErrs::E_NET, "server CreateIoCompletionPort failure!");
	}
	void IocpService::bind_session(Session* session)
	{
		if (m_state != state::net_running)
			return;

		if (session == nullptr)
			return;

		IOCP_DEBUG("iocp bind new session!");

		fd_t fd = session->get_fd();
		if (bind_socket(fd))
		{
			IOService::bind(session);

			IO_DATA* data = get_recv_data(session);
			data->m_fd = fd;
			data->m_owner = session;
			data->m_event = EventType::EV_READ;

			post_recv(data);
			return;
		}
		LogUtil::fatal((int32_t)EngineError::IOCP, "session CreateIoCompletionPort failure!");
	}

	void IocpService::Loop(int timeout)
	{
		DWORD dwTrans = 0;
		fd_t socket;
		IO_DATA* data;
		BOOL bRet;
		DWORD err;
		Session* session = nullptr;
		ServiceServer* server = nullptr;
		struct sockaddr* addrClient = nullptr, * addrLocal = nullptr;
		int client_len, local_len, addrlen;
		for (;m_state == state::net_stop;)
		{
			bRet = GetQueuedCompletionStatus(m_hiocp, &dwTrans, (PULONG_PTR)&socket, (LPOVERLAPPED*)&data, WSA_INFINITE);
			if (bRet)
			{
				if (data == NULL && socket == INVALID_SOCKET && dwTrans == -1)
				{
					IOCP_DEBUG("iocp thread exit by exit event!");
					break;
				}

				if (dwTrans == 0 && data->m_event != EventType::EV_ACCEPT)
				{
					IOCP_DEBUG("connection peer closed!");
					((Session*)data->m_owner)->close(close_reason::connect_peer_close);
					continue;
				}
				
				if (data->m_event == EventType::EV_READ) // recv data
				{
					session = (Session*)data->m_owner;
					if (session->process_recv(dwTrans))
					{
						post_recv(data);
					}
				}
				if (data->m_event == EventType::EV_WRITE) // send data
				{
					session = (Session*)data->m_owner;
					if (session->process_send(dwTrans))
					{
						post_send(data);
					}
				}
				if (data->m_event == EventType::EV_ACCEPT)
				{
					server = (ServiceServer*)(data->m_owner);
					setsockopt(data->m_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(socket), sizeof(socket));
					client_len = sizeof(sockaddr_storage), local_len = sizeof(sockaddr_storage), addrlen = sizeof(sockaddr_storage) + 16;
#if (1)
					m_accept_addrs(data->m_buffer.buf, 0, addrlen, addrlen,
						(LPSOCKADDR*)&addrLocal, &local_len, (LPSOCKADDR*)&addrClient, &client_len);
#else
					struct sockaddr addr;
					if (SOCKET_ERROR == getpeername(data->m_fd, &addr, &client_len))
					{
						IOCP_DEBUG("getpeername error! errno=%d", WSAGetLastError());
					}
					addrClient = &addr;
#endif
					ServiceServer* server = static_cast<ServiceServer*>(data->m_owner);
					server->process_accept(data, addrClient, &session);
					if (session)
					{
						bind_session(session);
					}
					post_accept(server, data);
				}
			}
			else
			{
				err = GetLastError();
				switch (err)
				{
				case WAIT_TIMEOUT:
					if (data->m_event == EventType::EV_ACCEPT)
					{
						IOCP_DEBUG("accept wait timeout!");
						server = (ServiceServer*)(data->m_owner);
						server->get_socket()->close_fd(data->m_fd);
						post_accept(server, data);
						break;
					}
					IOCP_DEBUG("connection wait timeout!");
					((Session*)data->m_owner)->close(close_reason::connect_timeout);
					break;
				case ERROR_NETNAME_DELETED:
				case ERROR_SEM_TIMEOUT:
					if (data->m_event == EventType::EV_ACCEPT)
					{
						IOCP_DEBUG("client close socket befor accept completing!");
						server = (ServiceServer*)(data->m_owner);
						server->get_socket()->close_fd(data->m_fd);
						post_accept(server, data);
						break;
					}
					IOCP_DEBUG("connection peer closed(ERROR_NETNAME_DELETED)!");
					((Session*)data->m_owner)->close(close_reason::connect_peer_close);
					break;
				case ERROR_OPERATION_ABORTED:
					IOCP_DEBUG("ERROR_OPERATION_ABORTED!");
					break;
				default:
					LogUtil::fatal(err, "iocp GetQueuedCompletionStatus failure!:%d", err);
					break;
				}
			}
		}
	}

	void IocpService::close()
	{
		m_threads.clear();
		CloseHandle(m_hiocp);
		m_hiocp = NULL;
		IOCP_DEBUG("destroy iocp handle!");
	}

	void IocpService::post_accept(ServiceServer* server, IO_DATA* data)
	{
		memset(&data->m_overlapped, 0, sizeof(data->m_overlapped));
		memset(data->m_buffer.buf, 0, data->m_buffer.len);
		ISocket* socket = server->get_socket();
		data->m_fd = socket->create_fd();
		DWORD byteReceived = 0;
		DWORD addrLen = sizeof(sockaddr_storage) + 16;
		if (!m_accept(server->get_fd(), data->m_fd, data->m_buffer.buf, 0,
			addrLen, addrLen, &byteReceived, (LPOVERLAPPED)data))
		{
			if (!server->is_running())
			{
				socket->close_fd(data->m_fd);
				return;
			}

			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err) {
				std::cout << "AcceptEx error errno=" << err << " !exit" << std::endl;
			}
		}
	}
	void IocpService::post_recv(IO_DATA* data)
	{
		memset(&data->m_overlapped, 0, sizeof(data->m_overlapped));
		DWORD dwRecv = 0;
		DWORD dwFlags = 0;
		if (WSARecv(data->m_fd, &data->m_buffer, 1, &dwRecv, &dwFlags, (OVERLAPPED*)data, NULL) == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAECONNABORTED || err == WSAECONNRESET)
			{
				IOCP_DEBUG("recv error: connection peer closed!");
				((Session*)data->m_owner)->close(close_reason::connect_peer_close);
				return;
			}

			if (err != WSA_IO_PENDING && err != WSAESHUTDOWN)
			{
				LogUtil::fatal((int32_t)EngineError::IOCP, "WSARecv error! erron=%d", err);
			}
		}
	}
	void IocpService::post_send(IO_DATA* data)
	{
		if (m_state != state::net_running)
			return;

		memset(&data->m_overlapped, 0, sizeof(data->m_overlapped));
		DWORD dwTrans = 0;
		DWORD dwFlags = 0;

		if (WSASend(data->m_fd, &data->m_buffer, 1, &dwTrans, dwFlags, (OVERLAPPED*)data, NULL) == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err == WSAECONNABORTED || err == WSAECONNRESET)
			{
				IOCP_DEBUG("send error: connection peer closed!");
				((Session*)data->m_owner)->close(close_reason::connect_peer_close);
				return;
			}

			if (err == WSAENOTSOCK)
			{
				LogUtil::warn("fd invalid!");
				return;
			}

			if (err != WSA_IO_PENDING && err != WSAESHUTDOWN)
			{
				LogUtil::fatal((int32_t)EngineError::IOCP, "WSASend error! erron=%d", err);
			}
		}
	}
}
#endif // _WIN
