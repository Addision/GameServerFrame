#include "net_base.hpp"

#ifdef _WIN
#pragma comment(lib, "ws2_32.lib") // windows编程需要的动态链接库
#endif
namespace Engine
{
	void NetBase::net_init()
	{
#ifdef _WIN
		WORD sockVersion = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(sockVersion, &data) != 0)
		{
			LogUtil::fatal(EngineErrs::E_NET, "net work init error!");
		}
		if (HIBYTE(data.wVersion) != 2 || LOBYTE(data.wVersion) != 2)
		{
			WSACleanup();
			LogUtil::fatal(EngineErrs::E_NET, "net work init error (Version != 2.2)!");
		}
#endif
	}

	void NetBase::net_free()
	{
#ifdef _WIN
		WSACleanup();
#endif
	}

	int NetBase::get_readable_size(const fd_t& fd)
	{
		unsigned long n_read = DEFAULT_SOCKET_READ_LEN;
		if (ioctlsocket(fd, FIONREAD, &n_read) < 0)
			return -1;
		return n_read;
	}

	int NetBase::get_socket_err(fd_t& fd)
	{
		int optval, optvallen = sizeof(optval);
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK && fd > 0)
		{
			if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optvallen))
				return err;
			if (optval)
				return optval;
		}
		return err;
	}

}
