#pragma once
#include "common/CommonDefines.hpp"
#include "engine/application.hpp"
#include "engine/com_singleton.hpp"
#include "engine/com_util.hpp"
#include "MasterServer/MasterNetServer.hpp"
#ifdef _WIN
#include "engine/io_iocp.hpp"	
#else
#include "engine/io_epoll.hpp"
#endif


class JsonConfig;
class MasterServer : public Application, public Singleton<MasterServer>
{
public:
	virtual bool OnStart();
	virtual void OnQuit();
private:
	void Init();
	void StartNetWork();
	void LoadConfig();
	void SetLog();
	//void SetLog();
	//void LoadConfig();
	//void LoadTables();
	//void CreateNetWork();
	//void CreateGameService();
// set log
// load config
// load tables
// create network
// create game module
private:
	MasterNetServer m_master_server;
	JsonConfig* m_json{nullptr};
};