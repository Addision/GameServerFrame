#include "client_base.hpp"
#include "node_client.hpp"
#include "logger.hpp"

namespace Engine
{
	ClientBase::~ClientBase()
	{
		delete m_node_client;
		m_node_client = nullptr;
	}
	void ClientBase::base_init()
	{
		m_node_client = new NodeClient;
		m_node_client->add_event_cb(ServerType::SERVER_MASTER, this, &ClientBase::on_socket_event);
		m_node_client->add_recv_cb(ServerType::SERVER_MASTER, MASTER_REPORT_SERVER_INFO_TO_SERVER, this, &ClientBase::on_master_message);;
	}

	void ClientBase::on_master_message(Session* session, IBuffer* buffer, int msg_id, int msg_len)
	{
		if (m_conn_list.empty()) return;
		ServerReportList report_list;
		// TODO: parse msg
		for (int i = 0; i < report_list.server_info_size(); ++i)
		{
			const ServerReport& server_info = report_list.server_info(i);
			for (auto& server_type : m_conn_list)
			{
				if (server_info.server_type() == server_type)
				{
					NodeServerInfoPtr node_server = std::make_shared<NodeServerInfo>();
					node_server->serv_id = server_info.server_id();
					node_server->port = server_info.server_port();
					node_server->serv_name = server_info.server_name();
					node_server->ip = server_info.server_ip();
					node_server->serv_type = ServerType(server_info.server_type());
					node_server->conn_state = ConnectState::NONE;
					m_node_client->add_conn_node(node_server);
				}
			}
		}
	}

	void ClientBase::on_socket_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base)
	{
		if (get_type() == ServerType::SERVER_MASTER) { return; }
		NodeServerInfoPtr node_server = get_node_info(net_base);
		if (net_event == NetEventType::NET_EV_CONNECTED && node_server)
		{
			node_server->fd = sock_fd;
			//m_node_client->send_pb_msg();
			//mNetCliModule->SendPbByServId(pConnData->serv_id, REPORT_CLIENT_INFO_TO_SERVER, &mServerInfo);
			//LOG_INFO("(%d : %s) report info to %s server", mServerInfo.server_id(), mServerInfo.server_name().c_str(), (pConnData->serv_name).c_str());
		}
		else
		{
			LogUtil::error("%s server client disconnected!!!", node_server->serv_name.c_str());
		}
	}

	void ClientBase::add_conn_master()
	{

	}

	void ClientBase::execute()
	{
		m_node_client->execute();
	}

}