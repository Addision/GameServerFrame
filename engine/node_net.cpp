#include "node_net.hpp"

namespace Engine
{
	NodeNet::NodeNet()
	{
	}

	NodeNet::~NodeNet()
	{
		stop();
	}
	bool NodeNet::create_server_service(const std::string& ip, uint32_t port)
	{
		m_net_service = new ServiceServer;
		m_io_service.set_net_service(m_net_service);
		m_net_service->set_cb(this, &NodeNet::on_recv_pack, &NodeNet::on_sock_event);
		m_net_service->set_io_service(&m_io_service);
		return m_net_service->start(ip.c_str(), port);
	}
	bool NodeNet::create_client_service(const std::string& ip, uint32_t port)
	{
		m_net_service = new ServiceClient;
		m_io_service.set_net_service(m_net_service);
		m_net_service->set_cb(this, &NodeNet::on_recv_pack, &NodeNet::on_sock_event);
		m_net_service->set_io_service(&m_io_service);
		return m_net_service->start(ip.c_str(), port);
	}

	void NodeNet::stop()
	{
		if (m_net_service)
		{
			m_net_service->stop();
			delete m_net_service;
			m_net_service = nullptr;
		}
	}

	void NodeNet::add_recv_cb(int msg_id, const recv_functor_ptr& cb)
	{
		auto it = m_recv_cb_map.find(msg_id);
		if (it != m_recv_cb_map.end())
		{
			return;
		}
		m_recv_cb_map.emplace(msg_id, cb);
	}

	void NodeNet::add_recv_cb(const recv_functor_ptr& cb)
	{
		m_recv_cb_list.emplace_back(cb);
	}

	void NodeNet::add_event_cb(const event_functor_ptr& cb)
	{
		m_event_cb_list.emplace_back(cb);
	}
	void NodeNet::on_recv_pack(Session* session, IBuffer* buffer, int msg_id, int msg_len)
	{
		auto it = m_recv_cb_map.find(msg_id);
		if (it != m_recv_cb_map.end())
		{
			it->second->operator()(session, buffer, msg_id, msg_len);
		}
		else
		{
			for (auto& it : m_recv_cb_list)
			{
				it->operator()(session, buffer, msg_id, msg_len);
			}
		}
	}

	void NodeNet::on_sock_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base)
	{
		for (auto& it : m_event_cb_list)
		{
			it->operator()(sock_fd, net_event, net_base);
		}
	}

}

