#include "MasterNetServer.hpp"
#include "engine/net_nodes.pb.h"
#include "engine/logger.hpp"

using namespace Engine;
MasterNetServer::MasterNetServer()
{
}

MasterNetServer::~MasterNetServer()
{
}

void MasterNetServer::init()
{
	ServerBase::base_init();
}

void MasterNetServer::clear()
{
	m_clients.clear();
	m_server_net.stop();
}

void MasterNetServer::after_report_server()
{
	ServerReportList report_list;
	for (auto it=m_clients.begin(); it!=m_clients.end(); it++)
	{
		ServerReport* server_info = report_list.add_server_info();
		server_info->CopyFrom(*(it->second->client_info));
	}
	if (report_list.server_info_size())
	{
		send_msg_all(MASTER_REPORT_SERVER_INFO_TO_SERVER, report_list);
	}

	// print sync node info
	LogUtil::info("=========================================================");
	for (int i=0;i<report_list.server_info_size();i++)
	{
		ServerReport server_info = report_list.server_info(i);
		LogUtil::info("(sync node info(%d : %s))", server_info.server_id(), server_info.server_name());
	}
	LogUtil::info("=========================================================");
}