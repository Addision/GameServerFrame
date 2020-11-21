#include "net_session.hpp"
#include "net_socket.hpp"
#include "net_service.hpp"
#include "io_service.hpp"


#define SESSION_LOG
#ifdef SESSION_LOG
#define SESSION_DEBUG(fmt,...) ENGINE_LOG(fmt,##__VA_ARGS__)
#else
#define SESSION_DEBUG(fmt,...)
#endif

namespace Engine
{
	void Session::init()
	{
		m_recv_buffer.init(RECV_BUFFER_SIZE); // default init buffer is 10*4096(40k)
		m_send_buffer.init(SEND_BUFFER_SIZE);
		int size = BLOCK_SIZE;
		m_recv_data.m_buffer.buf = m_recv_buffer.write(size);
		m_recv_data.m_buffer.len = size;
	}
	void Session::clear(void)
	{
		m_recv_buffer.clear();
		m_send_buffer.clear();
		close(SessionCloseType::none);
	}

	void Session::close(SessionCloseType reason)
	{
		conn_state exp = conn_state::connected;
		if (!m_state.compare_exchange_strong(exp, conn_state::closing))
		{
			return;
		}
		SESSION_DEBUG("session close %d", (int32_t)reason);
		m_close_reason = reason;
		m_socket.close();
		m_state = conn_state::none;
	}

	void Session::set_connected(INetService* net_service, const fd_t& fd, sockaddr* addr)
	{
		m_net_service = net_service;
		m_state = conn_state::connected;
		if (fd == INVALID_SOCKET) { return; } // it's invalid when called by client
		m_socket.set_fd(fd, addr); 
		m_msg_thread = m_net_service->m_dispatcher.get_thread(static_cast<int>(fd));
	}

	void Session::process_msg()
	{
		for (;;)
		{
			int buffer_size = m_recv_buffer.readable_size();
			if (buffer_size > MSG_HEADER_SIZE && m_net_service->m_recv_cb)
			{
				int msg_len = m_recv_buffer.get_msg_len();
				if (msg_len < MSG_HEADER_SIZE)
				{
					this->close(SessionCloseType::pack_error);
					break;
				}
				int msg_id = m_recv_buffer.get_msg_id();
				m_recv_buffer.commit_read(MSG_HEADER_SIZE);
				msg_len -= MSG_HEADER_SIZE;
				m_net_service->m_recv_cb(this, &m_recv_buffer, msg_id, msg_len);
				continue;
			}
			break;
		}
	}

	bool Session::process_recv(int size)
	{
		if (m_state != conn_state::connected)
			return false;
		m_recv_buffer.commit_write(size);
		m_msg_thread->add_task(std::bind(&Session::process_msg, this));

		int write_size = SOCKET_BUFFER_SIZE;
		m_recv_data.m_buffer.buf = m_recv_buffer.write(write_size);
		m_recv_data.m_buffer.len = write_size;
		if (m_recv_data.m_buffer.len == 0)
		{
			close(SessionCloseType::recv_buffer_overflow);
			return false;
		}

		return (m_state == conn_state::connected);
	}

	bool Session::process_send(int size)
	{
		if (m_state != conn_state::connected)
			return false;

		m_send_buffer.commit_read(size);
		size = BLOCK_SIZE;
		const char* p = m_send_buffer.read(size);

		if (size == 0 || m_state != conn_state::connected)
			return false;

		m_send_data.m_buffer.buf = const_cast<char*>(p);
		m_send_data.m_buffer.len = size;
		return true;
	}

	void Session::write_buffer(const char* msg, int len)
	{
		if (len <= 0) { return; }
		if (m_state != conn_state::connected) { return; }
		std::lock_guard<std::mutex> lock(m_send_mutex);
		if (m_send_buffer.writable_size() < len)
		{
			close(SessionCloseType::send_buffer_overflow);
			return;
		}
		char* p = nullptr;
		const char* data = msg;
		int left = len;
		int size = 0;
		for (; left != 0;)
		{
			size = left;
			p = m_send_buffer.write(size);
			memcpy(p, data, size);
			m_send_buffer.commit_write(size);
			data += size;
			left -= size;
		}
	}

	void Session::post_send()
	{
		int size = 0;
		m_send_data.m_buffer.buf = m_send_buffer.read(size);
		m_send_data.m_buffer.len = size;
		if (m_state == conn_state::connected)
		{
			m_io_service->post_send(&m_send_data);
		}
	}
	void Session::send_packet(int msg_id, const char* data, int len)
	{
		if (len > MAX_PROTOBUF_LEN)
		{
			LogUtil::error("message too large!");
			return;
		}
		MsgHeader header;
		header.msg_len = len + MSG_HEADER_SIZE;
		header.msg_id = msg_id;
		write_buffer((const char*)&header, MSG_HEADER_SIZE);
		write_buffer(data, len);
		post_send();
	}
}