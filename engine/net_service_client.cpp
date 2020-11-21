#include "net_service_client.hpp"
#include "net_session_factory.hpp"
#include "io_service.hpp"

namespace Engine
{
	ServiceClient::~ServiceClient()
	{
	}
	bool ServiceClient::start(const char* host, uint32_t port)
	{
		state exp = state::none;
		if (!m_state.compare_exchange_strong(exp, state::startting))
		{
			LogUtil::fatal(EngineErrs::E_NET, "client already running...");
			return false;
		}
		NetBase::net_init();
		m_io_service->set_net_service(this);
		m_session = SessionFactory::Instance()->malloc();
		if (m_session)
		{
			m_session->init();
			if (connect_server(host, port))
			{
				m_io_service->start();
				return true;
			}
			m_session->clear();
			m_session->close(SessionCloseType::none);
		}
		m_state = state::none;
		NetBase::net_free();
		return false;
	}

	void ServiceClient::stop()
	{
		state exp = state::connected;
		if (!m_state.compare_exchange_strong(exp, state::stopping))
		{
			return;
		}
		m_session->close(SessionCloseType::none);
		m_io_service->untrack_session(m_session);
		m_io_service->stop();
		NetBase::net_free();
	}

	bool ServiceClient::connect_server(const char* host, std::uint32_t port)
	{
		state exp = state::startting;
		if (!m_state.compare_exchange_strong(exp, state::connecting))
		{
			return false;
		}
		m_socket = m_session->get_socket();
		if (m_socket->connect(host, port, CONNECT_TIMEOUT))
		{
			m_session->set_connected(this, INVALID_SOCKET, nullptr);
			m_socket->set_opts();
			m_io_service->track_session(m_session);
			m_state = state::connected;
			return true;
		}
		return false;
	}

}