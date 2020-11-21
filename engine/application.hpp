#pragma once

#include "com_lock.hpp"
#include <list>
#include <string>

namespace Engine
{
	class Application
	{
		using param_list_t = std::list<std::string>;
	public:
		static void quit();
		bool start(int param_num, char* params[]);
		void run();
		static bool app_running;
	protected:
		virtual bool OnStart() = 0;
		virtual void OnQuit() = 0;
	private:
		static void yield();
		static void resume();
		void daemon();
		void hook_signal();
	private:
		param_list_t m_param_list;
		static cond_t m_cond;
		static mutex_t m_mutex;
	};
	void on_signal(int sig);
}
