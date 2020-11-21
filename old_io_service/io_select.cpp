#ifdef _SELECT
#include "io_select.hpp"

namespace Engine
{
	void Select::init()
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
	}

	void Select::add_event(fd_t& fd, int mask)
	{
		if (mask & EV_READ)
		{
			FD_SET(fd, &rfds);
		}
		if (mask & EV_WRITE)
		{
			FD_SET(fd, &wfds);
		}
	}

	void Select::del_event(fd_t& fd, int mask)
	{
		if (mask & EV_READ)
		{
			FD_CLR(fd, &rfds);
		}
		if (mask & EV_WRITE)
		{
			FD_CLR(fd, &wfds);
		}
	}

	bool Select::dispatch(int timeout)
	{
		int ret = 0;
		memcpy(&_rfds, &rfds, sizeof(rfds));
		memcpy(&_wfds, &wfds, sizeof(wfds));

		timeval* ptv = nullptr;
		timeval tv;
		if (timeout > 0)
		{
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			ptv = &tv;
		}
		ret = ::select((int)m_maxfd + 1, &_rfds, &_wfds, NULL, ptv);
		if (ret == -1)
		{
			if (SOCKET_ERR_RW_RETRIABLE(errno))
			{
				return true;
			}
			fprintf(stderr, "select error %d:%s", errno, strerror(errno));
			return false;
		}
		if (ret > 0) {
			int mask = 0;
			for (fd_t fd = 0; fd <= m_maxfd; fd++)
			{
				mask = 0;
				if (FD_ISSET(fd, &_rfds))
				{
					mask |= EV_READ;
					if (fd == m_serv_fd)
					{
						mask = EV_ACCEPT;
					}
				}
				if (FD_ISSET(fd, &_wfds))
				{
					mask |= EV_WRITE;
				}
				if (mask != 0)
				{
					set_event(fd, mask);
				}
			}
		}
		return true;
	}

	void Select::clear()
	{
		for (fd_t fd = 0; fd <= m_maxfd; fd++)
		{
			if (FD_ISSET(fd, &rfds))
			{
				FD_CLR(fd, &rfds);
			}
			if (FD_ISSET(fd, &wfds))
			{
				FD_CLR(fd, &wfds);
			}
		}
	}

	void Select::set_max_fd(fd_t& fd)
	{
		if (fd != INVALID_SOCKET && fd > m_maxfd)
		{
			m_maxfd = fd;
		}
	}
}

#endif