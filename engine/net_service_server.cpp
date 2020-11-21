#include "net_service_server.hpp"
#include "net_socket.hpp"
#include "net_session_factory.hpp"
#include "io_service.hpp"

namespace Engine
{
	ServiceServer::~ServiceServer()
	{
	}
	bool ServiceServer::start(const char* host, uint32_t port)
	{
		state exp = state::none;
		if (!m_serv_state.compare_exchange_strong(exp, state::running))
		{
			LogUtil::fatal(EngineErrs::E_NET, "server already running...");
		}
		NetBase::net_init();
		m_socket.bind(host, port); // 1¡¢create server socket 2¡¢bind addr 3¡¢set listen
		m_io_service->start();
		m_io_service->track_server(this);
		m_io_service->set_net_service(this);
		return true;
	}

	void ServiceServer::stop()
	{
		state exp = state::running;
		if (!m_serv_state.compare_exchange_strong(exp, state::stopping))
		{
			return;
		}
		for (auto& session : m_sessions)
		{
			session.second->close(SessionCloseType::none);
		}
		m_sessions.clear();
		m_io_service->untrack_server(this);
		m_io_service->stop();
		NetBase::net_free();
	}

	bool ServiceServer::process_accept(IO_DATA* data, sockaddr* addr, Session** out_session)
	{
		Session* session = SessionFactory::Instance()->malloc();
		if (session == nullptr)
		{
			m_socket.close_fd(data->m_fd);
			return false;
		}
		session->init();
		session->set_connected(this, data->m_fd, addr);
		session->get_socket()->set_opts();
		*out_session = session;
		save_session(session);
		m_event_cb(data->m_fd, NetEventType::NET_EV_CONNECTED, this);
		return true;
	}

	Session* ServiceServer::get_session(const fd_t& fd)
	{
		auto it = m_sessions.find(fd);
		if (it == m_sessions.end()) 
			return nullptr;
		return m_sessions[fd];
	}

	void ServiceServer::save_session(Session* session)
	{
		if (session == nullptr) return;
		m_sessions.emplace(session->get_fd(), session);
	}

	void ServiceServer::close_session(Session* session, SessionCloseType reason)
	{
		if (session == nullptr) return;
		m_sessions.erase(session->get_fd());
		SessionFactory::Instance()->free(session);
	}

}