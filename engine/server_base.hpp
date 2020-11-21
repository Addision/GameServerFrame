#pragma once
#include "net_defines.hpp"
#include "node_net.hpp"
#include "net_nodes.pb.h"
#include <memory>
#include <map>
namespace Engine
{
	struct NodeClientInfo
	{
		std::shared_ptr<ServerReport> client_info;
		fd_t fd;
	};
	using NodeClientInfoPtr = std::shared_ptr<NodeClientInfo>;
//////////////////////////////////////////////////////////////////////
	class ::google::protobuf::Message;
	class Session;
	class IBuffer;
	class NetBase;
	class ServerBase
	{
	public:
		virtual ~ServerBase();
		void base_init();
		virtual void init() = 0;
		virtual void clear() = 0;
		void execute();
		virtual void on_socket_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base);
		virtual void on_disconnect(const fd_t& sock_fd);
		virtual void on_connected(const fd_t& sock_fd);
		virtual void on_report_server(Session* session, IBuffer* buffer, int msg_id, int msg_len);
		virtual void after_report_server();

		NodeNet* get_net() { return &m_server_net; }
		NodeClientInfoPtr get_client_info(int server_id);
		NodeClientInfoPtr get_client_info(const fd_t& fd);
		// send functions
		void send_msg(ServerType server_type, int msg_id, const char* msg, int msg_len);
		void send_msg(int server_id, int msg_id, const char* msg, int msg_len);
		void send_msg(ServerType server_type, int msg_id, ::google::protobuf::Message& pb_msg);
		void send_msg(int server_id, int msg_id, ::google::protobuf::Message& pb_msg);
		void send_msg_all(int msg_id, const char* msg, int msg_len);
		void send_msg_all(int msg_id, ::google::protobuf::Message& pb_msg);
	protected:
		NodeNet m_server_net;
		// key:server id
		std::map<int, NodeClientInfoPtr> m_clients;
	};
}