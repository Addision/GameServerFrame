#include "application.hpp"
#include "engine_defines.hpp"
#include <signal.h>
#include <stdlib.h>
#ifdef _LINUX
#include <unistd.h>
#endif

#ifdef _WIN
#include <Windows.h>
#endif

namespace Engine
{
	cond_t Application::m_cond;
	mutex_t Application::m_mutex;
	bool Application::app_running = false;

	void on_signal(int sig)
	{
		switch (sig)
		{
		case SIGINT:
		case SIGTERM:
#ifdef _WIN
		case SIGABRT:
		case SIGBREAK:
#endif			
			Application::app_running = false;
			Application::quit();
			break;
		default:
			break;
		}
	}
	bool Application::start(int param_num, char* params[])
	{
		Application::app_running = true;
		bool is_deamon = false;
		for (size_t i=0;i<param_num;i++)
		{
			if (strcmp(params[i], "-d") == 0 || strcmp(params[i], "-daemon") == 0)
				is_deamon = true;
			m_param_list.push_back(params[i]);
		}
		if (is_deamon) { daemon(); }
		hook_signal();
		return OnStart();
	}

	void Application::run()
	{
		while (app_running)
		{
			yield();
		}
		OnQuit();
	}

	void Application::quit()
	{
		if (!app_running)
		{
			resume();
		}
	}

	void Application::yield()
	{
		unique_lock_t lock(m_mutex);
		m_cond.wait(lock);
	}

	void Application::resume()
	{
		m_cond.notify_all();
	}

	void Application::daemon()
	{
#ifdef _LINUX
		int pid = fork();
		if (pid != 0)
		{
			_exit(-1);
		}
		setsid();
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
#endif
	}

	void Application::hook_signal()
	{
#ifndef _WIN
		signal(SIGHUP, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
		act.sa_handler = on_signal;
		sigaction(SIGINT, &act, NULL);
		sigaction(SIGTERM, &act, NULL);
#else
		signal(SIGINT, on_signal);
		signal(SIGTERM, on_signal);
		signal(SIGABRT, on_signal);
		signal(SIGBREAK, on_signal);
#endif
	}

}
