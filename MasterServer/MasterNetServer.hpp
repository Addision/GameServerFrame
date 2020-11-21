#pragma once

#include "common/CommonDefines.hpp"
#include "engine/Application.hpp"
#include "engine/server_base.hpp"

class MasterNetServer : public Engine::ServerBase
{
public:
	MasterNetServer();
	~MasterNetServer();
	virtual void init() override;
	virtual void clear() override;
	virtual void after_report_server() override;
private:

private:
};