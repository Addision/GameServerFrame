#ifdef _EPOLL
#include "net_epoll.hpp"

namespace Engine
{
	void Epoll::init()
	{
		m_epfd = ::epoll_create(EVENTS_NUM);
	}
	void Epoll::add_event(fd_t& fd, int mask)
	{
		uint32_t events = EPOLLET | EPOLLONESHOT;
		if (mask & EV_READ)
		{
			events |= EPOLLIN;
		}
		if (mask & EV_WRITE)
		{
			events |= EPOLLOUT;
		}

		auto it = m_events_map.find(fd);
		if (it == m_events_map.end())
		{
			struct epoll_event ev = { 0 };
			ev.data.fd = fd;
			ev.events = events; // EPOLLET | EPOLLONESHOT | EPOLLIN | EPOLLOUT
			m_events_map.emplace(fd, ev);
			epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev)
		}
		else
		{
			it->second.events = events;
			epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev)
		}
	}

	void Epoll::del_event(fd_t& fd, int mask)
	{
		auto it = m_events_map.find(fd);
		if (it != m_events_map.end())
		{
			epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr)
		}
	}
	bool Epoll::dispatch(int timeout)
	{
		int ret = 0;
		int events_num = 0;
		int tm_out = -1;
		if (timeout > 0)
		{
			tm_out = timeout * 1000;
		}
		ret = epoll_wait(m_epfd, m_events, n_events, tm_out); // msec
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
					if (ev.events & EPOLLIN)
					{
						mask |= EV_READ;
						if (ev.data.fd == m_serv_fd)
						{
							mask = EV_ACCEPT;
						}
						add_event(ev.data.fd, mask);
					}
					if (ev.events & EPOLLOUT)
					{
						mask |= EV_WRITE;
						add_event(ev.data.fd, mask);
					}
					if (ev.events & EPOLLRDHUP)   // 对端关闭
					{
						mask |= EV_CLOSED;
						del_event(ev.data.fd, EPOLLRDHUP);
					}
					if (ev.events & EPOLLONESHOT) // 重新添加
					{
						mask |= EV_READ;
						add_event(ev.data.fd, mask);
						continue;
					}
				}
				if (mask != 0)
				{
					set_event(ev.data.fd, mask);
				}
			}
		}
		return true;
	}
	void Epoll::clear()
	{
		shutdown(m_epfd, SHUT_RDWR);
		close(m_epfd);
		m_epfd = INVALID_SOCKET;
	}
}
#endif