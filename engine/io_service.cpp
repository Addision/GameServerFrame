#include "io_service.hpp"
#include "net_session.hpp"
#include "com_util.hpp"

namespace Engine
{
	void IOService::bind(Session* session)
	{
		session->m_io_service = this;
	}

	IO_DATA* IOService::get_send_data(Session* session)
	{
		return &session->m_send_data;
	}

	IO_DATA* IOService::get_recv_data(Session* session)
	{
		return &session->m_recv_data;
	}

	void IOService::service_start()
	{
		m_state = state::net_start;
		n_threads = get_cpu_cores() + 1;
	}

	void IOService::service_stop()
	{
		m_state = state::net_stop;
	}
}