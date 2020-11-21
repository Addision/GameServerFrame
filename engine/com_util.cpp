#include "com_util.hpp"

#ifdef _WIN
#include <windows.h>
#pragma comment(lib,"kernel32.lib")
#else
#include <unistd.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#endif

namespace Engine
{
	void localtime_r(const time_t* timep, struct tm* result)
	{
#ifdef _WIN
		localtime_s(result, timep);
#endif
	}

	int get_cpu_cores()
	{
		int cpu_cores = 0;
#ifdef _WIN
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		cpu_cores = sysInfo.dwNumberOfProcessors;
#else
		cpu_cores = sysconf(_SC_NPROCS_CONF);
#endif
		return cpu_cores;
	}

	void set_resource()
	{
#ifdef _LINUX
		// set dump core size
		struct rlimit rlim, rlim_new;
		if (getrlimit(RLIMIT_CORE, &rlim) == 0)
		{
			rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
			if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
			{
				rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
				(void)setrlimit(RLIMIT_CORE, &rlim_new);
			}
		}
		// set can open max file num
		if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
		{
			rlim_new.rlim_cur = rlim_new.rlim_max = 8192;
			if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0)
			{
				rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
				(void)setrlimit(RLIMIT_NOFILE, &rlim_new);
			}
		}
#endif
	}
}