#pragma once
#include "com_singleton.hpp"
#include "mem_data_factory.hpp"
#include "net_session.hpp"

namespace Engine
{
	class SessionFactory : public Singleton<SessionFactory>
	{
	public:  
		Session* malloc()
		{
			return m_session_pool.malloc();
		}
		void free(Session* session)
		{
			m_session_pool.free(session);
		}
	private:
		MemData::DataPool<Session, SESSION_INIT_SIZE, SESSION_GROW_SIZE> m_session_pool;
	};
}