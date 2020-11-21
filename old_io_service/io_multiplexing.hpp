/*
 * @Author: jia.lai
 * @Date: 2020-09-16 14:05:27
 * @LastEditTime: 2020-09-16 15:37:33
 * @Description: 
 * @Version: 1.0
 */
#pragma once

#include "net_defines.hpp"
#include "com_thread_pool.hpp"
#ifdef _LINUX
// linux io ʹ�ýӿ�

namespace Engine
{
	class ServiceServer;
	class Session;
	class IEventOp
	{
		friend class Multiplexing;

	public:
		virtual ~IEventOp(void);
		virtual void init() = 0;
		virtual void add_event(fd_t &fd, int mask) = 0;
		virtual void del_event(fd_t &fd, int mask) = 0;
		virtual bool dispatch(int timeout = 0) = 0;
		virtual void clear() = 0;

		void set_event(fd_t &fd, int mask);
		void set_serv_fd(fd_t &fd);

	protected:
		std::map<fd_t, int> m_events;
		fd_t m_serv_fd{INVALID_SOCKET};
	};
	////////////////////////////////////////////////////////////////////////////////////////
	class Multiplexing : public IOService
	{
		using func_t = std::function<void()>;

	public:
		virtual void start() override;
		virtual void stop() override;
		virtual void Loop(int timeout = 0) override;
		virtual void bind_server(ServiceServer *server) override;
		virtual void bind_session(Session *session) override;

		void event_read(Session *session);
		void event_write(Session *session);
		void event_accept(AcceptData *data);
		void process_event(fd_t &fd, int event, Session *session, AcceptData *data);
		void close_session(Session *session, close_reason reason);

	private:
		IEventOp *m_eventop{nullptr};
		AcceptData *m_data{nullptr};

		ThreadPool<func_t> m_threads;
	};

} // namespace Engine

#endif // _LINUX