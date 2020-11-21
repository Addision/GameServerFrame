#pragma once
#include "net_defines.hpp"
#include <list>
#include <atomic>
#include <thread>
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
		virtual bool start() = 0;
		virtual void stop() = 0;
		virtual void loop() = 0;
		virtual void track_server(ServiceServer* server) {}
		virtual void untrack_server(ServiceServer* server) {}
		virtual void track_session(Session* session) {}
		virtual void untrack_session(Session* session) {}

		void bind(Session* session);
		IO_DATA* get_send_data(Session* session);
		IO_DATA* get_recv_data(Session* session);
		void set_net_service(INetService* net_service) { m_net_service = net_service; }

		virtual void post_send(IO_DATA* data) {}
		virtual void post_recv(IO_DATA* data) {}

	protected:
		void service_start();
		void service_stop();
	protected:
		std::atomic<state> m_state = state::net_none;
		std::int32_t n_threads{ 0 };
		std::list<std::thread> m_threads;
		INetService* m_net_service{nullptr};
	};
}