#pragma once
#include "net_defines.hpp"
#include "io_iocp.hpp"
#include "io_epoll.hpp"
#include "net_service_client.hpp"
#include "net_service_server.hpp"

namespace Engine
{
	using namespace std::placeholders;
	class INetService;
	class NodeNet
	{
		friend class ServerBase;
		friend class ClientBase;
		friend class NodeClient;
	public:
		NodeNet();
		virtual ~NodeNet();
		bool create_server_service(const std::string& ip, uint32_t port);
		bool create_client_service(const std::string& ip, uint32_t port);

		// add call back function
		template<typename BaseType>
		void add_recv_cb(const int msg_id, BaseType* base_type, void(BaseType::*handle_recv)(Session* session, IBuffer* buffer, int msg_id, int msg_len))
		{
			recv_functor functor = std::bind(handle_recv, base_type, _1, _2, _3, _4);
			recv_functor_ptr functor_ptr = std::make_shared<recv_functor>(functor);
			add_recv_cb(msg_id, functor_ptr);
		}
		template<typename BaseType>
		void add_recv_cb(BaseType* base_type, void(BaseType::* handle_recv)(Session* session, IBuffer* buffer, int msg_id, int msg_len))
		{
			recv_functor functor = std::bind(handle_recv, base_type, _1, _2, _3, _4);
			recv_functor_ptr functor_ptr = std::make_shared<recv_functor>(functor);
			add_recv_cb(functor_ptr);
		}
		template<typename BaseType>
		void add_event_cb(BaseType* base_type, void(BaseType::*handle_event)(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base))
		{
			event_functor functor = std::bind(handle_event, base_type, _1, _2, _3);
			event_functor_ptr functor_ptr = std::make_shared<event_functor>(functor);
			add_event_cb(functor_ptr);
		}
		// stop net
		void stop();

		void add_recv_cb(int msg_id, const recv_functor_ptr& cb);
		void add_recv_cb(const recv_functor_ptr& cb);
		void add_event_cb(const event_functor_ptr& cb);

	private:
		// called by net_service
		void on_recv_pack(Session* session, IBuffer* buffer, int msg_id, int msg_len);
		void on_sock_event(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base);
	protected:
		std::map<int, recv_functor_ptr> m_recv_cb_map;	// msg id   msg handle
		std::list<recv_functor_ptr> m_recv_cb_list;
		std::list<event_functor_ptr> m_event_cb_list;

		INetService* m_net_service{ nullptr };
		EpollService m_io_service;
	};
}
