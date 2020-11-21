#include "mem_stream_buffer.hpp"
#include "com_lock.hpp"
#include  <climits>
#include <cassert>

namespace Engine
{
	StreamBuffer::StreamBuffer(void)
	{

	}
	StreamBuffer::~StreamBuffer(void)
	{
	}
	void StreamBuffer::init(int size)
	{
		assert(size >= block_size);
		int node_count = size / block_size;
		m_factory.init(size % block_size ? node_count + 1 : node_count);
		
		m_head = m_factory.malloc();
		m_head->init();
		m_tail = m_head;
		m_next = m_head;
		m_readable = 0;
		m_proto_size = 0;
		m_proto_read = 0;
		m_offset = 0;
	}

	void StreamBuffer::clear()
	{
		m_factory.clear();
	}

	char* StreamBuffer::read(int& size)
	{
		size = (int)(m_head->m_writer - m_head->m_reader);
		return m_head->m_reader;
	}

	bool StreamBuffer::commit_read(int size)
	{
		assert(size <= m_readable);
		m_readable -= size;
		int len =(int) (m_head->m_writer - m_head->m_reader);
		if (size <= len) {
			m_head->m_reader += size;
			return true;
		}
		stream_node_t* tmp = nullptr;
		for (;;)
		{
			size -= len;
			if (size < 0)
			{
				m_head->m_reader = m_head->m_reader + size;
				break;
			}
			else
			{
				tmp = m_head;
				m_head = m_head->next_node;
				m_factory.free(tmp);
			}
			len = block_size;
		}

		return true;
	}

	char* StreamBuffer::write(int& size)
	{
		size = (int)(m_tail->m_buffer + block_size - m_tail->m_writer);
		return m_tail->m_writer;
	}

	void StreamBuffer::commit_write(int size)
	{
		m_tail->m_writer += size;
		m_readable += size;

		if (m_tail->m_writer >= m_tail->m_buffer + block_size) {
			m_tail->next_node = m_factory.malloc();
			m_tail = m_tail->next_node;
			m_tail->init();
		}
	}

	int StreamBuffer::readable_size()
	{
		return m_readable;
	}

	int StreamBuffer::writable_size()
	{
		return ULONG_MAX;
	}

	const char* StreamBuffer::next(int& size)
	{
		int limit = m_proto_size > 0 ? m_proto_size : readable_size();
		assert(m_proto_size <= limit);
		if (limit <= m_proto_read) {
			size = 0;
			return nullptr;
		}
		return _next(size, limit);
	}

	bool StreamBuffer::skip(int size)
	{
		int limit = m_proto_size > 0 ? m_proto_size : readable_size();
		assert(m_proto_size <= limit);

		if (size > limit - m_proto_read)
			return false;

		int left = size;
		do {
			size = left;
			_next(size, limit);
			left -= size;
		} while (left > 0);

		return true;
	}
	const char* StreamBuffer::_next(int& size, int limit)
	{
		int left = limit - m_proto_read;
		if (m_offset >= block_size)
		{
			m_offset = 0;
			m_next = m_next->next_node;
		}

		limit = block_size - m_offset;
		if (left > limit) { left = limit; }
		if (size == 0 || size > left) { size = left; }
		left = m_offset;
		m_proto_read += size;
		m_offset += size;

		return m_next->m_buffer + left;
	}
	bool StreamBuffer::back_up(int size)
	{
		if (size > m_proto_read) { return false; }

		if (size <= m_offset)
		{
			m_proto_read -= size;
			m_offset -= size;
			return true;
		}

		// performs warning! 
		size = m_proto_read - size;
		reset();
		if (size == 0) return true;
		int limit = m_proto_size;
		m_proto_size = 0;
		skip(size);
		m_proto_size = limit;
		return true;
	}
	void StreamBuffer::reset()
	{
		m_proto_read = 0;
		m_offset = (int)(m_head->m_reader - m_head->m_buffer);
		m_next = m_head;
	}

	int StreamBuffer::get_msg_id()
	{
		return parse_msg_head();
	}

	int StreamBuffer::get_msg_len()
	{
		int msg_len = parse_msg_head();
		this->set_proto_size(msg_len);
		return msg_len;
	}

	int StreamBuffer::parse_msg_head()
	{
		int pos = 0;
		int key = 0;
		int size = sizeof(int);
		int len = 0;
		for (;;)
		{
			const char* p = next(len);
			if (!p) { return false; }
			memcpy((char*)(&key) + pos, p, len);
			pos += len;
			if (pos >= size) {
				break;
			}
			len = size - pos;
		}
		return key;
	}
}