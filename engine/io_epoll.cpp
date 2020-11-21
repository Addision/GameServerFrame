#include "io_epoll.hpp"
#ifdef _EPOLL

#define IO_LOG
#ifdef IO_LOG
#define IO_DEBUG(fmt,...) ENGINE_DEBUG(fmt,##__VA_ARGS__)
#else
#define IO_DEBUG(fmt,...)
#endif
namespace Engine
{
	bool EpollService::start()
	{
		IOService::service_start();
		create_epoll();
		m_events = new event_array[n_threads];
		m_threads.init(n_threads);
		m_loop_timeout = LOOP_TIMEOUT;

		return true;
	}

	void EpollService::stop()
	{
		state st = state::net_running;
		if (!m_state.compare_exchange_strong(st, state::net_stop))
		{
			if (st == state::net_start)
			{
				LogUtil::fatal(EngineErrs::E_NET, "stop io_service_epoll on starting!");
			}
			IO_DEBUG("already stopping!");
			return;
		}
		for (std::thread& it : m_threads)
		{
			it.join();
		}
		delete[] m_events;
		closesocket(m_epfd);
		IO_DEBUG("epoll service stop...");
		m_state = state::net_none;
		IO_DEBUG("iocp stopped!");
	}

	void EpollService::create_epoll()
	{
		m_epfd = ::epoll_create(EVENTS_NUM);
	}
	void EpollService::add_event(fd_t& fd, int events, int op)
	{
		struct epoll_event ev = { 0 };
		ev.data.fd = fd;
		ev.events = events; // EPOLLET | EPOLLONESHOT | EPOLLIN | EPOLLOUT
		epoll_ctl(m_epfd, op, fd, &ev);
	}

	void EpollService::del_event(fd_t& fd, int mask)
	{
		struct epoll_event ev = { 0,{0} };
		epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ev);
	}

	bool EpollService::dispatch(epoll_event* events)
	{
		int ret = 0;
		int events_num = 0;
		int tm_out = (int)LOOP_TIMEOUT * 1000;
		ret = epoll_wait(m_epfd, events, n_events, tm_out); // msec
		if (ret == -1)
		{
			if (SOCKET_ERR_RW_RETRIABLE(errno))
			{
				return true;
			}
			LogUtil::fatal(EngineErrs::E_NET, "epoll_wait error %d:%s", errno, strerror(errno));
			return false;
		}
		if (ret > 0)
		{
			events_num = ret;
			for (int i = 0; i < events_num; i++)
			{
				struct epoll_event ev = m_events[i];
				int mask = 0;
				if (ev.events & (EPOLLHUP | EPOLLERR)) // 出现这两个标识通过read|write检查socket
				{
					mask = EV_READ | EV_WRITE;
				}
				else
				{
					if (ev.events & EPOLLRDHUP)   // 对端关闭
					{
						mask |= EV_CLOSED;
						del_event(ev.data.fd, EPOLLRDHUP);
					}
					if (ev.events & EPOLLIN)
					{
						if (ev.data.fd == m_serv_fd)
						{
							mask = EV_ACCEPT;
							add_event(ev.data.fd, EPOLLET|EPOLLONESHOT|EPOLLIN|EPOLLOUT, EPOLL_CTL_ADD);
						}
						else
						{
							mask |= EV_READ;
							add_event(ev.data.fd, EPOLLET|EPOLLONESHOT|EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD);
						}
					}
					if (ev.events & EPOLLOUT)
					{
						mask |= EV_WRITE;
						add_event(ev.data.fd, EPOLLET|EPOLLONESHOT|EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD);
					}

					if (ev.events & EPOLLONESHOT) // 重新添加
					{
						add_event(ev.data.fd, EPOLLET|EPOLLONESHOT|EPOLLIN|EPOLLOUT, EPOLL_CTL_ADD);
						continue;
					}
				}
				if (mask != 0)
				{
					event_map[ev.data.fd] = mask;
				}
			}
		}
		return true;
	}

	void EpollService::process_event(epoll_event* events)
	{
		m_state = state::net_running;
		Session* session = nullptr;
		std::map<fd_t, int> event_map;

		for (; m_state != state::net_stop;)
		{
			if (!dispatch(events))
			{
				LogUtil::fatal(EngineErrs::E_NET, "io_multiplexing dispatch error");
				break;
			}
			for (auto& ev : event_map)
			{
				fd_t fd = ev.first;
				int mask = ev.second;
				if (mask & EventType::EV_ACCEPT)
				{
					event_accept(data);
					continue;
				}
				Session* session = m_net_service->get_session(fd);
				if (!session) { return; }
				if (mask & EventType::EV_READ)
				{
					event_read(session);
				}
				if (mask & EventType::EV_WRITE)
				{
					event_write(session);
				}
				if (mask & EventType::EV_CLOSED)
				{
					close_session(session);
				}
			}
			event_map.clear();
		}
	}

	void EpollService::loop()
	{
		for (int i = 0; i < n_threads; ++i)
		{
			struct epoll_event* events = m_events[i]->events;
			m_threads.emplace_back(std::thread(std::bind(&EpollService::process_event, this, events)));
		}
	}

	// call bind_server when start net service
	void EpollService::track_server(ServiceServer* server)
	{
		if (m_state != state::net_running)
			return;
		IO_DEBUG("muliplexing io bind_server");
		m_data = server->get_accept_data();
		m_data->m_overlapped.m_inuse = false;
		m_data->m_overlapped.m_need_send = false;
		m_data->m_event = EventType::EV_ACCEPT;
		m_data->m_owner = server;
		ISocket* socket = server->get_socket();
		socket->set_nonblock();
		m_listen_fd = socket->get_fd();
		m_data->m_fd = m_listen_fd;

		m_data->m_buffer.buf = m_data->m_buff;
		m_data->m_buffer.len = sizeof(m_data->m_buff);

		add_event(m_listen_fd, EPOLLET | EPOLLONESHOT | EPOLLIN | EPOLLOUT, EPOLL_CTL_ADD);
	}

	void EpollService::untrack_server(ServiceServer* server)
	{
		del_event(m_listen_fd, EPOLLIN);
	}

	void EpollService::track_session(Session* session)
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
		add_event(data->m_fd, EPOLLET | EPOLLONESHOT | EPOLLIN | EPOLLOUT, EPOLL_CTL_ADD);
	}
	void EpollService::untrack_session(Session* session)
	{
		del_event(session->get_fd(), EPOLLIN);
	}
	void EpollService::event_accept(AcceptData* data)
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
				add_event(conn_fd, EventType::EV_READ);
				server->process_accept(data, &addr, &session); // create session
				if (session) { track_session(session); }
				continue;
			}
			break;
		}
	}
	void EpollService::event_read(Session* session)
	{
		if (session == nullptr) return;
		fd_t fd = session->get_fd();
		IO_DEBUG("read event trigger, socket %d", fd);
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
					IO_DEBUG("recv error, connection peer closed!");
					close_session(session, close_reason::connect_peer_close);
					return;
				}
				IO_DEBUG("socket recv error! err code %d, %s", errno, strerror(errno));
				close_session(session, close_reason::recv_unknow_err);
				return;
			}
			else if (ret == 0)  // read eof
			{
				IO_DEBUG("recv error, connection peer closed!");
				close_session(session, close_reason::connect_peer_close);
				break;
			}
			left_size -= ret;
			session->process_recv(ret);
			if (left_size == 0) { break; }
		}
	}
	void EpollService::event_write(Session* session)
	{
		if (session == nullptr) return;
		fd_t fd = session->get_fd();
		IO_DEBUG("write event trigger, socket %d", fd);
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
					IO_DEBUG("send error, connection peer closed!");
					close_session(session, close_reason::connect_peer_close);
					return;
				}
				IO_DEBUG("socket send error! err code %d, %s", errno, strerror(errno));
				close_session(session, close_reason::send_unknow_err);
				break;
			}
			session->process_send(ret);
		}
		del_event(fd, EV_WRITE);
	}

	void EpollService::close_session(Session* session, close_reason reason)
	{
		session->close(reason);
		m_net_service->del_session(session);
	}

}
#endif