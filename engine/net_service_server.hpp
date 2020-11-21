#pragma once
#include "net_service.hpp"
#include "mem_data_factory.hpp"
#include "net_socket.hpp"
#include <atomic>
#include <map>
namespace Engine
{
	class Session;
	class IOService;
	class ServiceServer : public INetService
	{
		enum class state {none, running, stopping};
	public:
		virtual ~ServiceServer();
		virtual bool start(const char* host, uint32_t port) override;
		virtual void stop() override;
		virtual bool process_accept(IO_DATA* data, sockaddr* addr, Session** session);

		AcceptData* get_accept_data() { return m_accept_data.malloc(); }
		ISocket* get_socket() { return &m_socket; }
		fd_t get_fd() { return m_socket.get_fd(); }
		bool is_running() { return m_serv_state==state::running; }

		virtual Session* get_session(const fd_t& fd) override;
		virtual void close_session(Session* session, SessionCloseType reason) override;
		void save_session(Session* session);

	private:
		static constexpr int IOCP_ACCEPT_NUM = 10;
		MemData::DataFactory<AcceptData, IOCP_ACCEPT_NUM> m_accept_data;
		Socket m_socket;
		std::atomic<state> m_serv_state = state::none;

		std::map<fd_t, Session*> m_sessions;
	};
}