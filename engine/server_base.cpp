#include "server_base.hpp"
#include "node_net.hpp"
#include "net_session.hpp"
#include "logger.hpp"

namespace Engine
{
	ServerBase::~ServerBase()
	{
		
	}

	void ServerBase::base_init()
	{
		m_server_net.add_recv_cb(REPORT_CLIENT_INFO_TO_SERVER, this, &ServerBase::on_report_server);
		m_server_net.add_event_cb(this, &ServerBase::on_socket_event);
	}
	void ServerBase::execute()
	{
		m_server_net.m_io_service.loop();
	}
	void ServerBase::on_socket_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base)
	{
		switch (net_event)
		{
		case NetEventType::NET_EV_CONNECTED:
			on_connected(sock_fd);
			break;
		case NetEventType::NET_EV_EOF:
		case NetEventType::NET_EV_ERROR:
		case NetEventType::NET_EV_TIMEOUT:
		{
			on_disconnect(sock_fd);
			break;
		}
		}
	}
	void ServerBase::on_disconnect(const fd_t& sock_fd)
	{
		LogUtil::info("client:%d disconnect");
		for (auto it = m_clients.begin(); it!= m_clients.end();)
		{
			if (it->second->fd == sock_fd)
			{
				it = m_clients.erase(it);
				continue;
			}
			it++;
		}
	}
	void ServerBase::on_connected(const fd_t& sock_fd)
	{
		LogUtil::info("client:%d connect server ok");
	}
	void ServerBase::on_report_server(Session* session, IBuffer* buffer, int msg_id, int msg_len)
	{
		ServerReport report;

		// TODO parse msg
		NodeClientInfoPtr node_client_info = std::make_shared<NodeClientInfo>();
		node_client_info->fd = session->get_fd();
		node_client_info->client_info = std::make_shared<ServerReport>(report);
		m_clients.emplace(report.server_id(), node_client_info);

		for (auto& it : m_clients)
		{
			LogUtil::info("client:(%d, %s) report info to server", it.first, it.second->client_info->server_name().c_str());
		}

		after_report_server();  // called by master server
	}
	void ServerBase::after_report_server()
	{
	}
	NodeClientInfoPtr ServerBase::get_client_info(int server_id)
	{
		auto it = m_clients.find(server_id);
		if (it == m_clients.end()) { return nullptr; }
		return it->second;
	}

	Engine::NodeClientInfoPtr ServerBase::get_client_info(const fd_t& fd)
	{
		for (auto& it : m_clients)
		{
			if (fd == it.second->fd)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	void ServerBase::send_msg(ServerType server_type, int msg_id, const char* msg, int msg_len)
	{
		for (auto& it : m_clients)
		{
			auto type = it.second->client_info->server_type();
			if (type == server_type)
			{
				auto session = m_server_net.m_net_service->get_session(it.second->fd);
				session->send_packet(msg_id, msg, msg_len);
			}
		}
	}
	void ServerBase::send_msg(int server_id, int msg_id, const char* msg, int msg_len)
	{
		auto it = m_clients.find(server_id);
		if (it == m_clients.end()) { return; }
		auto session = m_server_net.m_net_service->get_session(it->second->fd);
		session->send_packet(msg_id, msg, msg_len);
	}
	void ServerBase::send_msg(ServerType server_type, int msg_id, ::google::protobuf::Message& pb_msg)
	{
		for (auto& it : m_clients)
		{
			auto type = it.second->client_info->server_type();
			if (type == server_type)
			{
				auto session = m_server_net.m_net_service->get_session(it.second->fd);
				session->send_packet(msg_id, pb_msg);
			}
		}
	}
	void ServerBase::send_msg(int server_id, int msg_id, ::google::protobuf::Message& pb_msg)
	{
		auto it = m_clients.find(server_id);
		if (it == m_clients.end()) { return; }
		auto session = m_server_net.m_net_service->get_session(it->second->fd);
		session->send_packet(msg_id, pb_msg);
	}

	void ServerBase::send_msg_all(int msg_id, const char* msg, int msg_len)
	{
		for (auto& it : m_clients)
		{
			auto session = m_server_net.m_net_service->get_session(it.second->fd);
			session->send_packet(msg_id, msg, msg_len);
		}
	}

	void ServerBase::send_msg_all(int msg_id, ::google::protobuf::Message& pb_msg)
	{
		for (auto& it : m_clients)
		{
			auto session = m_server_net.m_net_service->get_session(it.second->fd);
			session->send_packet(msg_id, pb_msg);
		}
	}
}