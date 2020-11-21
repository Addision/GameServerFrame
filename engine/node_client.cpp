#include "node_client.hpp"
#include "net_session.hpp"
#include <chrono>

namespace Engine
{
	NodeClient::NodeClient()
	{
		for (int serv_type = 0; serv_type < ServerType::SERVER_MAX; ++serv_type)
		{
			CallBack cb;
			m_cb_map.emplace(serv_type, cb);
		}
	}
	NodeClient::~NodeClient()
	{
		m_cb_map.clear();
	}
	void NodeClient::add_conn_node(NodeServerInfoPtr& server_data)
	{
		guard_lock_t lock(m_mutex);
		m_temp_list.emplace_back(server_data);
	}
	NodeServerInfoPtr NodeClient::get_node_info(const fd_t& sock_fd)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->fd == sock_fd)
			{
				return it.second;
			}
		}
		return nullptr;
	}
	NodeServerInfoPtr NodeClient::get_node_info(const int& server_id)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->serv_id == server_id)
			{
				return it.second;
			}
		}
		return nullptr;
	}
	NodeServerInfoPtr NodeClient::get_node_info(const NetBase* net)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->net_ptr->m_net_service == net)
			{
				return it.second;
			}
		}
		return nullptr;
	}
	void NodeClient::send_msg(ServerType server_type, int msg_id, const char* msg, int msg_len)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->serv_type == server_type && it.second->conn_state == ConnectState::CONNOK)
			{
				send_packet(it.second, msg_id, msg, msg_len);
			}
		}
	}
	void NodeClient::send_msg(int server_id, int msg_id, const char* msg, int msg_len)
	{
		auto it = m_conn_servers.find(server_id);
		if (it == m_conn_servers.end()) { return; }
		if (it->second->conn_state == ConnectState::CONNOK)
		{
			send_packet(it->second, msg_id, msg, msg_len);
		}
	}
	void NodeClient::send_msg(ServerType server_type, int msg_id, ::google::protobuf::Message& pb_msg)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->serv_type == server_type && it.second->conn_state == ConnectState::CONNOK)
			{
				send_packet(it.second, msg_id, pb_msg);
			}
		}
	}
	void NodeClient::send_msg(int server_id, int msg_id, ::google::protobuf::Message& pb_msg)
	{
		auto it = m_conn_servers.find(server_id);
		if (it == m_conn_servers.end()) { return; }
		if (it->second->conn_state == ConnectState::CONNOK)
		{
			send_packet(it->second, msg_id, pb_msg);
		}
	}
	void NodeClient::send_msg_all(int msg_id, const char* msg, int msg_len)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->conn_state == ConnectState::CONNOK)
			{
				send_packet(it.second, msg_id, msg, msg_len);
			}
		}
	}
	void NodeClient::send_msg_all(int msg_id, ::google::protobuf::Message& pb_msg)
	{
		for (auto& it : m_conn_servers)
		{
			if (it.second->conn_state == ConnectState::CONNOK)
			{
				send_packet(it.second, msg_id, pb_msg);
			}
		}
	}
	void NodeClient::execute()
	{
		m_thread_loop = std::thread(std::bind(&NodeClient::loop, this));
	}

	void NodeClient::loop()
	{
		for (;;)
		{
			process_loop();
			process_connect();
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	void NodeClient::process_loop()
	{
		for (auto it = m_conn_servers.begin(); it!= m_conn_servers.end();)
		{
			switch (it->second->conn_state)
			{
			case ConnectState::NONE:
			{
				// create client service and connect server
				if (it->second->net_ptr->create_client_service(it->second->ip, it->second->port))
				{
					it->second->conn_state = ConnectState::CONNECTED;
				}
				else
				{
					it->second->conn_state = ConnectState::RECONNECT;
				}
				break;
			}
			case ConnectState::CONNECTED:
			{
				it->second->net_ptr->m_io_service.loop();
				it->second->conn_state = ConnectState::CONNOK;
				break;
			}
			case ConnectState::RECONNECT:
			{
				it->second->net_ptr = std::make_shared<NodeNet>();
				m_conn_servers.emplace(it->second->serv_id, it->second);
				it->second->conn_state = ConnectState::NONE;
				it->second->m_conn_num++;
				if (it->second->m_conn_num >= 3)  // 尝试链接超过3次不在链接
				{
					it = m_conn_servers.erase(it);
					continue;
				}
				break;
			}
			case ConnectState::CONNOK:
				break;
			}
			it++;
		}
	}
	void NodeClient::process_connect()
	{
		guard_lock_t lock(m_mutex);
		if (m_temp_list.empty()) { return; }
		for (auto& conn_ptr : m_temp_list)
		{
			auto it = m_conn_servers.find(conn_ptr->serv_id);
			if (it == m_conn_servers.end())
			{
				conn_ptr->net_ptr = std::make_shared<NodeNet>();
				init_node_cbs(conn_ptr);
				m_conn_servers.emplace(conn_ptr->serv_id, conn_ptr);
			}
		}
		m_temp_list.clear();
	}
	void NodeClient::add_recv_cb(ServerType serv_type, int msg_id, const recv_functor_ptr& cb)
	{
		auto it = m_cb_map.find(serv_type);
		if (it == m_cb_map.end())
		{
			return;
		}
		m_cb_map[serv_type].m_recv_cb_map.emplace(msg_id, cb);
	}
	void NodeClient::add_recv_cb(ServerType serv_type, const recv_functor_ptr& cb)
	{
		auto it = m_cb_map.find(serv_type);
		if (it == m_cb_map.end())
		{
			return;
		}
		m_cb_map[serv_type].m_recv_cb_list.emplace_back(cb);
	}

	void NodeClient::add_event_cb(ServerType serv_type, const event_functor_ptr& cb)
	{
		auto it = m_cb_map.find(serv_type);
		if (it == m_cb_map.end())
		{
			return;
		}
		m_cb_map[serv_type].m_event_cb_list.emplace_back(cb);
	}
	void NodeClient::init_node_cbs(NodeServerInfoPtr& server_data)
	{
		for (auto& callback : m_cb_map)
		{
			for (auto& it : callback.second.m_recv_cb_map)
			{
				server_data->net_ptr->add_recv_cb(it.first, it.second);
			}
			for (auto& it : callback.second.m_recv_cb_list)
			{
				server_data->net_ptr->add_recv_cb(it);
			}
			for (auto& it : callback.second.m_event_cb_list)
			{
				server_data->net_ptr->add_event_cb(it);
			}
		}
	}

	void NodeClient::send_packet(const NodeServerInfoPtr& node_ptr, int msg_id, const char* msg, int msg_len)
	{
		auto net_service = node_ptr->net_ptr->m_net_service;
		auto session = net_service->get_session(node_ptr->fd);
		session->send_packet(msg_id, msg, msg_len);
	}

	void NodeClient::send_packet(const NodeServerInfoPtr& node_ptr, int msg_id, ::google::protobuf::Message& pb_msg)
	{
		auto net_service = node_ptr->net_ptr->m_net_service;
		auto session = net_service->get_session(node_ptr->fd);
		session->send_packet(msg_id, pb_msg);
	}

}