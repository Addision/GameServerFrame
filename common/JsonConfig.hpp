#pragma once
#include "CommonDefines.hpp"
#include "engine/com_singleton.hpp"
#include "json.h"

class JsonConfig : public Engine::Singleton<JsonConfig>
{
public:
	bool Load(const char* json_file);

	Json::Value m_root;
	Json::Value m_kMasterConf;
	Json::Value m_kGameConf;
	Json::Value m_kWorldConf;
	Json::Value m_kMessageConf;
	Json::Value m_kDatabaseConf;
	Json::Value m_kGatePlayerConf;
	Json::Value m_kMysqlConf;
};
