#pragma once
#include "io_service.hpp"
#ifdef _IOCP
//// ÊµÏÖ²Î¿¼£ºhttps://blog.csdn.net/wojiuguowei/article/details/82658790?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param
namespace Engine
{
	class ServiceServer;
	class Session;
	class IocpService : public IOService
	{
	public:
		virtual bool start() override;
		virtual void stop() override;
		virtual void loop() override;
		virtual void track_server(ServiceServer* server) override;
		virtual void track_session(Session* session) override;
		virtual void post_send(IO_DATA* data) override;
		virtual void post_recv(IO_DATA* data) override;
	private:
		bool bind_socket(fd_t& fd);
		void creat_iocp();
		void process_event();
		void post_accept(ServiceServer* server, IO_DATA* data);
		void close();
	private:
		HANDLE m_hiocp{nullptr};
		LPFN_ACCEPTEX m_accept{nullptr};
		LPFN_GETACCEPTEXSOCKADDRS m_accept_addrs{nullptr};
	};
////////////////////////////////////////////////////////////////////////////////////////////////
	using EpollService = IocpService;
}

#endif // _IOCP
