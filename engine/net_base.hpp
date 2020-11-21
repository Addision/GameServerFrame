#pragma once

#include "net_defines.hpp"
#include "logger.hpp"
#include <memory>
#include <functional>

namespace Engine
{
	class NetBase
	{
	public:
		void net_init();
		void net_free();
		static int get_readable_size(const fd_t& fd);
		static int get_socket_err(fd_t& fd);
	private:
	};

////////////////////////////////////////////////////////////////////////////////////////////////

	class Session;
	class IBuffer;

	using recv_handler_t = void(Session* session, IBuffer* buffer, int msg_id, int msg_len);
	using event_handler_t = void(const fd_t& sock_fd, const NetEventType net_event, NetBase* net_base);
	using recv_functor = std::function<recv_handler_t>;
	using recv_functor_ptr = std::shared_ptr<recv_functor>;
	using event_functor = std::function<event_handler_t>;
	using event_functor_ptr = std::shared_ptr<event_functor>;


}
