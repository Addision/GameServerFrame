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
		state st = state::net_none;
		if (!m_state.compare_exchange_strong(st, state::net_start))
			return;
		n_threads = get_cpu_cores() + 1;
	}

	void IOService::service_stop()
	{
		state st = state::net_running;
		if (!m_state.compare_exchange_strong(st, state::net_stop))
			return;

	}

}