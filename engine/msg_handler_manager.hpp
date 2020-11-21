#pragma once

#include <unordered_map>
#include "msg_defines.hpp"
#include "mem_buffer.hpp"
#include "net_session.hpp"

namespace Engine
{
	using handler_t = void(Session* session, IBuffer* buffer, int msg_id, int msg_len);
	template<class key_t = int>
	class HandlerManager
	{
		using handler_map_t = typename std::unordered_map<key_t, handler_t>;
	public:
		handler_t get_handler(key_t key);
		handler_t get_handler(IBuffer* buffer);
		void attach(key_t key, handler_t handle);
	private:
		handler_map_t m_handlers;
	};
	template<class key_t>
	inline handler_t HandlerManager<key_t>::get_handler(key_t key)
	{
		typename handler_map_t::iterator it = m_handlers.find(key);
		if (it != m_handlers.end())
			return it->second;
		return nullptr;
	}
	template<class key_t>
	handler_t HandlerManager<key_t>::get_handler(IBuffer* buffer)
	{
		key_t key;
		int size = sizeof(key_t);
		const char* p;
		int len = size;
		int pos = 0;
		do
		{
			p = msg->next(len);
			if (!p) return nullptr;
			memcpy((char*)(&key) + pos, p, len);
			pos += len;
			if (pos >= size) break;
			len = size - pos;
		} while (true);

		typename handler_map_t::iterator it = m_handlers.find(key);
		if (it != m_handlers.end())
			return it->second;

		return nullptr;
	}
	template<class key_t>
	void HandlerManager<key_t>::attach(key_t key, handler_t handle)
	{
		typename handler_map_t::iterator it = m_handlers.find(key);
		if (it != m_handlers.end())
			return;

		m_handlers[key] = handle;
	}
}