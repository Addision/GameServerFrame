#pragma once

#include "net_service.hpp"
#include "net_socket.hpp"
#include "net_defines.hpp"
#include <atomic>
namespace Engine
{
	class Session;
	class ServiceClient : public INetService
	{
		enum class state { none, startting, connecting, connected, timeout, stopping };
	public:
		virtual ~ServiceClient();
		virtual bool start(const char* host, std::uint32_t port) override;
		virtual void stop() override;

		virtual Session* get_session(const fd_t& fd) override { return m_session; }
		ISocket* get_socket() { return m_socket; }
		fd_t get_fd() { return m_socket->get_fd(); }
		bool is_connected() { return m_state==state::connected; }
	private:
		bool connect_server(const char* host, std::uint32_t port);
	private:
		Session* m_session;
		ISocket* m_socket;
		std::atomic<state> m_state = state::none;
	};
}