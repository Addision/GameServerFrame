#pragma once
#include "net_defines.hpp"
#include "msg_defines.hpp"
#include "msg_dispatcher.hpp"
#include "net_socket.hpp"
#include "mem_protobuf.hpp"
#include "mem_stream_buffer.hpp"


namespace Engine
{
	class IOService;
	class INetService;
	class Session
	{
		friend class IOService;
		enum class conn_state { none, connected, closing };
	public:
		Session() {}
		~Session() {}
		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;

		void init();
		void clear();
		void close(SessionCloseType reason);
		bool is_connected() { return m_state == conn_state::connected; }
		fd_t get_fd() { return m_socket.get_fd(); }
		ISocket* get_socket() { return &m_socket; }
		void set_connected(INetService* net_service, const fd_t& fd, struct sockaddr* addr);

		IO_DATA* get_recv_data() { return &m_recv_data; }
		IO_DATA* get_send_data() { return &m_send_data; }
		bool process_recv(int size);
		bool process_send(int size);

		void send_packet(int msg_id, const char* data, int len);
		template<typename T>
		void send_packet(int msg_id, const T& data)
		{
			static_assert(std::is_base_of<google::protobuf::Message, T>::value, "data mast be google::protobuf::Message");
			int data_size = (int)data.ByteSizeLong();
			if (data_size > MAX_PROTOBUF_LEN)
			{
				LogUtil::error("message too large!");
				return ;
			}
			MsgHeader header;
			header.msg_id = msg_id;
			header.msg_len = data_size + MSG_HEADER_SIZE;
			write_buffer((const char*)&header, MSG_HEADER_SIZE);
			OutputStream stream(&m_send_buffer);
			data.SerializeToZeroCopyStream(&stream);
			this->post_send();
		}
	private:
		void write_buffer(const char* msg, int len);
		void post_send();
		void process_msg();
	private:
		std::atomic<conn_state> m_state;
		Socket m_socket;
		IO_DATA m_recv_data;
		IO_DATA m_send_data;
		SessionCloseType m_close_reason;

		StreamBuffer m_recv_buffer;
		StreamBuffer m_send_buffer;
		std::mutex m_send_mutex;

		IOService* m_io_service{nullptr};
		INetService* m_net_service{nullptr};

		ThreadWrap<func_t>* m_msg_thread{nullptr};
	};
}