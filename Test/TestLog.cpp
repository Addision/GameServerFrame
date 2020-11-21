#include "GameTest.hpp"
#include "engine/logger.hpp"
#include "common/LogSystem.hpp"
//
using namespace Engine;

void GameTest::TestLog()
{
	try
	{
		SysLog* pSysLog = SysLog::Instance();
		pSysLog->Start(LogLevel::DEBUG);
		g_pLog->set_log(pSysLog);

		LogInfo << "xxxxxxxxxxxxxxxx" << LogEnd;
		LogInfo << "xxxxxxxxxxxxxxxxfffffffffff" << LogEnd;
	}
	catch (std::exception& e)
	{
		
	}
}