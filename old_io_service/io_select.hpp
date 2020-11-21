#pragma once
#ifdef _SELECT
#include "net_multiplexing.hpp"
#include <sys/select.h>

namespace Engine
{
	class Select : public IEventOp
	{
	public:
		virtual void init();
		virtual void add_event(fd_t& fd, int mask);
		virtual void del_event(fd_t& fd, int mask);
		virtual bool dispatch(int timeout = 0);
		virtual void clear();

		void set_max_fd(fd_t& fd);
	private:
		fd_set rfds, wfds;
		fd_set _rfds, _wfds;

		fd_t m_maxfd{ -1 };
	};
}

#endif