#include "MasterServer.hpp"

int main(int argc, char* argv[])
{
	set_resource();
	MasterServer* master = MasterServer::Instance();
	if (master->start(argc, argv))
	{
		LogUtil::info("master server start...");
		master->run();
	}

#ifdef _WIN
	system("pause");
#endif
	return 0;
}