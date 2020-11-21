#pragma once
#include "net_nodes.pb.h"
#include "net_defines.hpp"
#include "node_client.hpp"
#include <list>
namespace Engine
{
	class Session;
	class IBuffer;
	class NodeClient;
	class NetBase;
	class ClientBase
	{
	public:
		virtual ~ClientBase();
		void base_init();
		virtual void init() = 0;
		virtual void clear() = 0;
		virtual void set_report_info() = 0;
		virtual void add_conn_server() = 0;
		virtual void on_master_message(Session* session, IBuffer* buffer, int msg_id, int msg_len);
		virtual void on_socket_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base);
		virtual void add_conn_master();
		void execute();
		ServerType get_type() { return (ServerType)m_server_info.server_type(); }
		NodeServerInfoPtr get_node_info(const fd_t& sock_fd) {return m_node_client->get_node_info(sock_fd);}
		NodeServerInfoPtr get_node_info(const int& server_id) {return m_node_client->get_node_info(server_id);}
		NodeServerInfoPtr get_node_info(const NetBase* net) {return m_node_client->get_node_info(net);}
		NodeClient* get_node_client() {return m_node_client;}
	protected:
		ServerReport m_server_info;
		NodeClient* m_node_client;
		std::list<ServerType> m_conn_list;
	};
}