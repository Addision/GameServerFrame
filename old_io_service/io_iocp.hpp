#pragma once

#include "net_defines.hpp"
#ifdef _WIN
#include "io_service.hpp"
#include <thread>
#include <list>
#include <atomic>

// ÊµÏÖ²Î¿¼£ºhttps://blog.csdn.net/wojiuguowei/article/details/82658790?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.channel_param
namespace Engine
{
	class ServiceServer;
	class Session;
	class IocpService : public IOService
	{
	public:
		IocpService(void);
		~IocpService(void);

		IocpService(const IocpService&) = delete;
		IocpService& operator=(const IocpService&) = delete;
	public:
		virtual void start() override;
		virtual void stop() override;
		virtual void bind_server(ServiceServer* server) override;
		virtual void bind_session(Session* session) override;
		virtual void Loop(int timeout = 0) override;
	private:
		bool bind_socket(fd_t& fd);
		void creat_iocp();
		void close();

		void post_accept(ServiceServer* server, IO_DATA* data);
		void post_recv(IO_DATA* data);
		void post_send(IO_DATA* data);
	private:
		HANDLE m_hiocp{nullptr};
		LPFN_ACCEPTEX m_accept{nullptr};
		LPFN_GETACCEPTEXSOCKADDRS m_accept_addrs{nullptr};
	};
}

#endif // _WIN
