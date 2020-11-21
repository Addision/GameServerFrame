#pragma once
#include "io_service.hpp"
#include "com_thread_pool.hpp"
#ifdef _EPOLL
#include <sys/epoll.h>

namespace Engine
{
	class EpollService : public IOService
	{

		static constexpr int n_events = 255;
		struct event_array
		{
			struct epoll_event events[n_events];
		};
	public:
		virtual bool start() override;
		virtual void stop() override;
		virtual void loop() override;
		virtual void track_server(ServiceServer* server);
		virtual void untrack_server(ServiceServer* server);
		virtual void track_session(Session* session);
		virtual void untrack_session(Session* session);

		void event_read(Session* session);
		void event_write(Session* session);
		void event_accept(AcceptData* data);
		void close_session(Session* session, close_reason reason);
	private:
		void create_epoll();
		void process_event(epoll_event* events);
		bool dispatch(epoll_event* events);
		void add_event(fd_t& fd, int mask, int op);
		void del_event(fd_t& fd, int mask);
	private:
		fd_t m_epfd{ -1 };
		struct event_array* m_events;
		AcceptData* m_data{ nullptr };

		int m_loop_timeout{ 0 };
		fd_t m_listen_fd{INVALID_SOCKET};
	};
}

#endif