#pragma once
#ifdef _EPOLL
#include "net_multiplexing.hpp"
#include <sys/epoll.h>
#include <map>
namespace Engine
{
	class Epoll : public IEventOp
	{
	public:
		virtual void init();
		virtual void add_event(fd_t& fd, int mask);
		virtual void del_event(fd_t& fd, int mask);
		virtual bool dispatch(int timeout = 0);
		virtual void clear();
	private:
		static constexpr int n_events = 255;
		fd_t m_epfd{-1};
		struct epoll_event[n_events] m_events;
		std::map<fd_t, struct epoll_event> m_events_map;
	}
}

#endif