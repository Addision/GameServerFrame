#include "MasterServer.hpp"
#include "common/JsonConfig.hpp"
#include "common/LogSystem.hpp"

bool MasterServer::OnStart()
{
	try
	{
		SetLog();
		Init();
		StartNetWork();

	}
	catch (std::exception& e)
	{
		LogUtil::error("start master error, %s", e.what());
		return false;
	}
	return true;
}

void MasterServer::OnQuit()
{
	m_master_server.clear();
}

void MasterServer::Init()
{
	LoadConfig();
	m_master_server.init();
}

void MasterServer::StartNetWork()
{
	if (m_master_server.get_net()->create_server_service(m_json->m_kMasterConf["ip"].asString(), m_json->m_kMasterConf["port"].asUInt()))
	{
		m_master_server.execute();
	}
	else
	{
		throw std::runtime_error("start Master Server Net error");
	}
}

void MasterServer::LoadConfig()
{
	m_json = JsonConfig::Instance();
	if (m_json->Load(SERVER_CONF))
	{
		m_json->m_kMasterConf = m_json->m_root["MasterServer"];
	}
	else
	{
		throw std::runtime_error("load config err, can't find server config file");
	}
}

void MasterServer::SetLog()
{
	SysLog* pSysLog = SysLog::Instance();
	pSysLog->Start(LogLevel::DEBUG);
	LogUtil::set_log(pSysLog);
}
