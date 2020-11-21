#pragma once
#include "node_net.hpp"
#include "com_lock.hpp"
#include "google/protobuf/message.h"
#include <string>

namespace Engine
{
	enum class ConnectState
	{
		NONE = 1,
		CONNECTED,
		RECONNECT,
		CONNOK,
	};

	// 客户端需要连接服务器信息
	struct NodeServerInfo
	{
		int serv_id;
		int port;
		std::string ip;
		std::string serv_name;
		ServerType serv_type;
		ConnectState conn_state;
		fd_t fd;
		std::shared_ptr<NodeNet> net_ptr;
		std::atomic_int m_conn_num = 0;
	};
	using NodeServerInfoPtr = std::shared_ptr<NodeServerInfo>;
////////////////////////////////////////////////////////////////////////////////////////
	class NodeClient
	{
	public:
		NodeClient();
		~NodeClient();
		void add_conn_node(NodeServerInfoPtr& server_data);
		void execute();
		// add callback functions
		template<typename BaseType>
		void add_recv_cb(ServerType serv_type, const int msg_id, BaseType* base_type, void(BaseType::*recv_handler)(Session* session, IBuffer* buffer, int msg_id, int msg_len))
		{
			recv_functor functor = std::bind(recv_handler, base_type, _1, _2, _3, _4);
			recv_functor_ptr functor_ptr = std::make_shared<recv_functor>(functor);
			add_recv_cb(serv_type, msg_id, functor_ptr);
		}
		template<typename BaseType>
		void add_recv_cb(ServerType serv_type, BaseType* base_type, void(BaseType::*recv_handler)(Session* session, IBuffer* buffer, int msg_id, int msg_len))
		{
			recv_functor functor = std::bind(recv_handler, base_type, _1, _2, _3, _4);
			recv_functor_ptr functor_ptr = std::make_shared<recv_functor>(functor);
			add_recv_cb(serv_type, functor_ptr);
		}
		template<typename BaseType>
		void add_event_cb(ServerType serv_type, BaseType* base_type, void(BaseType::*event_handler)(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base))
		{
			event_functor functor = std::bind(event_handler, base_type, _1, _2, _3);
			event_functor_ptr functor_ptr = std::make_shared<event_functor>(functor);
			add_event_cb(serv_type, functor_ptr);
		}
		NodeServerInfoPtr get_node_info(const fd_t& sock_fd);
		NodeServerInfoPtr get_node_info(const int& server_id);
		NodeServerInfoPtr get_node_info(const NetBase* net);

		// send function
		void send_msg(ServerType server_type, int msg_id, const char* msg, int msg_len);
		void send_msg(int server_id, int msg_id, const char* msg, int msg_len);
		void send_msg(ServerType server_type, int msg_id, ::google::protobuf::Message& pb_msg);
		void send_msg(int server_id, int msg_id, ::google::protobuf::Message& pb_msg);
		void send_msg_all(int msg_id, const char* msg, int msg_len);
		void send_msg_all(int msg_id, ::google::protobuf::Message& pb_msg);
	private:
		void loop();
		void process_loop();
		void process_connect();

		void add_recv_cb(ServerType serv_type, int msg_id, const recv_functor_ptr& cb);
		void add_recv_cb(ServerType serv_type, const recv_functor_ptr& cb);
		void add_event_cb(ServerType serv_type, const event_functor_ptr& cb);

		void init_node_cbs(NodeServerInfoPtr& server_data);

		void send_packet(const NodeServerInfoPtr& node_ptr, int msg_id, const char* msg, int msg_len);
		void send_packet(const NodeServerInfoPtr& node_ptr, int msg_id, ::google::protobuf::Message& pb_msg);
	private:
		struct CallBack
		{
			std::map<int, recv_functor_ptr> m_recv_cb_map;
			std::list<recv_functor_ptr> m_recv_cb_list;
			std::list<event_functor_ptr> m_event_cb_list;
		};
		// key is server type
		std::map<int, CallBack> m_cb_map;
		// key is server id
		std::map<int, NodeServerInfoPtr> m_conn_servers;
		std::list<NodeServerInfoPtr> m_temp_list;
		std::thread m_thread_loop;
		mutex_t m_mutex;
	};
}