#ifdef _LINUX
#include "net_multiplexing.hpp"
#include "net_service.hpp"
#include "net_session.hpp"
#include "net_service_server.hpp"
#include "net_service_client.hpp"
#include "com_util.hpp"

#ifdef _EPOLL
#include "io_epoll.hpp"
#else
#include "io_select.hpp"
#endif

namespace Engine
{
	IEventOp::~IEventOp(void)
	{
		m_events.clear();
	}

	void IEventOp::set_event(fd_t& fd, int mask)
	{
		m_events[fd] = mask;
	}

	void IEventOp::set_serv_fd(fd_t& fd)
	{
		if (fd != INVALID_SOCKET)
		{
			m_serv_fd = fd;
		}
	}
//////////////////////////////////////////////////////////////////////////////////
	void Multiplexing::start()
	{
		IOService::service_start();
#if defined _EPOLL
		m_eventop = new Epoll();
#elif defined _SELECT
		m_eventop = new Select();
#endif
		m_eventop->init();
		m_threads.init(n_threads);
		this->Loop(LOOP_TIMEOUT);
	}

	void Multiplexing::stop()
	{
		IOService::service_stop();
	}

	void Multiplexing::Loop(int timeout)
	{
		Session* session = nullptr;
		for (;m_state != state::net_stop;)
		{
			if (!m_eventop->dispatch(timeout))
			{
				LogUtil::fatal(EngineErrs::E_NET, "io_multiplexing dispatch error");
				break;
			}
			for (auto& ev : m_eventop->m_events)
			{
				fd_t fd = ev.first;
				int event = ev.second;
				Session* session = m_net_service->get_session(fd);
				m_threads.add_task(std::bind(&Multiplexing::process_event, this, fd, event, session, m_data));
			}
			m_eventop->m_events.clear();
		}
	}
	void Multiplexing::process_event(fd_t& fd, int event, Session* session, AcceptData* data)
	{
		if (event & EventType::EV_ACCEPT)
		{
			event_accept(data);
		}
		if (!session) return;
		if (event & EventType::EV_READ)
		{
			event_read(session);
		}
		if (event & EventType::EV_WRITE)
		{
			event_write(session);
		}
		if (event & EventType::EV_CLOSED)
		{
			close_session(session);
		}
	}
	// call bind_server when start net service
	void Multiplexing::bind_server(ServiceServer* server)
	{
		if (m_state != state::net_running)
			return;
		LogUtil::info("muliplexing io bind_server");
		m_data = server->get_accept_data();
		m_data->m_overlapped.m_inuse = false;
		m_data->m_overlapped.m_need_send = false;
		m_data->m_event = EventType::EV_ACCEPT;
		m_data->m_owner = server;
		ISocket* socket = server->get_socket();
		socket->set_nonblock();
		m_data->m_fd = socket->get_fd();

		m_data->m_buffer.buf = m_data->m_buff;
		m_data->m_buffer.len = sizeof(m_data->m_buff);

		m_eventop->add_event(m_data->m_fd, EventType::EV_READ);
	}

	void Multiplexing::bind_session(Session* session)
	{
		IOService::bind(session);

		fd_t fd = session->get_fd();

		IO_DATA* data = get_send_data(session);
		data->m_overlapped.m_inuse = false;
		data->m_overlapped.m_need_send = false;
		data->m_event = EventType::EV_NONE;
		data->m_owner = session;
		data->m_fd = fd;

		data = get_recv_data(session);
		data->m_overlapped.m_inuse = false;
		data->m_overlapped.m_need_send = false;
		data->m_event = EventType::EV_NONE;
		data->m_owner = session;
		data->m_fd = fd;
	}
	void Multiplexing::event_accept(AcceptData* data)
	{
		if (data == nullptr) return;
		ServiceServer* server = static_cast<ServiceServer*>(data->m_owner);
		ISocket* socket = server->get_socket();
		for (;;)
		{
			struct sockaddr addr;
			socklen_t len = sizeof(addr);
			fd_t conn_fd = socket->accept(&addr, len);
			if (conn_fd != INVALID_SOCKET)
			{
				m_eventop->add_event(conn_fd, EventType::EV_READ);
				server->process_accept(data, &addr, &session); // create session
				if (session) bind_session(session);
				continue;
			}
			break;
		}
	}
	void Multiplexing::event_read(Session* session)
	{
		if (session == nullptr) return;
		fd_t fd = session->get_fd();
		NET_DEBUG("read event trigger, socket %d", fd);
		IO_DATA* data = get_recv_data(session);
		int recv_size = NetBase::get_readable_size(fd);
		int left_size = recv_size;
		for (; left_size >= 0;)
		{
			int ret = ::recv(fd, data.m_buffer.buf, left_size, 0);
			if (ret < 0)
			{
				int err = NetBase::get_socket_err(fd);
				if (SOCKET_ERR_RW_RETRIABLE(err)) { break; }
				if (SOCKET_CONN_CLOSE(err))
				{
					NET_DEBUG("recv error, connection peer closed!");
					close_session(session, close_reason::connect_peer_close);
					return;
				}
				NET_DEBUG("socket recv error! err code %d, %s", errno, strerror(errno));
				close_session(session, close_reason::recv_unknow_err);
				return;
			}
			else if (ret == 0)  // read eof
			{
				NET_DEBUG("recv error, connection peer closed!");
				close_session(session, close_reason::connect_peer_close);
				break;
			}
			left_size -= ret;
			session->process_recv(ret);
			if (left_size == 0) { break; }
		}
	}
	void Multiplexing::event_write(Session* session)
	{
		if (session == nullptr) return;
		fd_t fd = session->get_fd();
		NET_DEBUG("write event trigger, socket %d", fd);
		IO_DATA* data = get_send_data(session);
		while (session && data->m_buffer.buf && data->m_buffer.len)
		{
			int ret = ::send(fd, data->m_buffer.buf, data->m_buffer.len, 0);
			if (ret < 0)
			{
				int err = NetBase::get_socket_err(fd);
				if (SOCKET_ERR_RW_RETRIABLE(err)) { break; }
				if (SOCKET_CONN_CLOSE(err))
				{
					NET_DEBUG("send error, connection peer closed!");
					close_session(session, close_reason::connect_peer_close);
					return;
				}
				NET_DEBUG("socket send error! err code %d, %s", errno, strerror(errno));
				close_session(session, close_reason::send_unknow_err);
				break;
			}
			session->process_send(ret);
		}
		m_eventop->del_event(fd, EV_WRITE);
	}

	void Multiplexing::close_session(Session* session, close_reason reason)
	{
		m_eventop->del_event(session->get_fd(), EV_READ | EV_WRITE);
		session->close(reason);
		m_net_service->del_session(session);
	}
}

#endif  //_LINUX