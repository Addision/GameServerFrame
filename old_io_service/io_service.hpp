#pragma once
#include "net_defines.hpp"

namespace Engine
{
	class Session;
	class INetService;
	class ServiceServer;
	class IOService
	{
	public:
		enum class state {
			net_none = 0,
			net_start,
			net_running,
			net_stop,
		};
		virtual void start() {}
		virtual void stop() {}
		virtual void Loop(int timeout = 0) = 0;
		virtual void bind_server(ServiceServer* server) {}
		virtual void bind_session(Session* session) {}
		virtual void unbind_session(Session* session) {}

		void bind(Session* session);
		IO_DATA* get_send_data(Session* session);
		IO_DATA* get_recv_data(Session* session);
		void set_net_service(INetService* net_service) { m_net_service = net_service; }
	protected:
		void service_start();
		void service_stop();
	protected:
		std::atomic<state> m_state;
		std::int32_t n_threads{ 0 };
		std::list<std::thread> m_threads;
		INetService* m_net_service{ nullptr };
	};
}