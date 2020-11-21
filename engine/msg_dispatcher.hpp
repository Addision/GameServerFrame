#pragma once
// 分发处理消息
#include "com_thread_pool.hpp"

namespace Engine
{
	template<class T>
	class ThreadWrap
	{
		using task_t = typename std::conditional<std::is_same<func_t, T>::value, T, Task<T>>::type;
	public:
		void start()
		{
			m_thread = std::thread(std::bind(&ThreadWrap::loop, this));
		}
		void stop()
		{
			unique_lock_t lock(m_mutex);
			m_tasks.clear();
			lock.unlock();
			m_cv.notify_all();
			if (m_thread.joinable())
			{
				m_thread.join();
			}
		}
		void add_task(task_t&& task)
		{
			unique_lock_t lock(m_mutex);
			m_tasks.emplace_back(task);
			m_cv.notify_all();
		}
		template<class F, typename... Args>
		void add_task(F&& f, Args&&... args)
		{
			this->add_task(func_t(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
		}
	private:
		task_t get_task()
		{
			unique_lock_t lock(m_mutex);
			while (m_tasks.empty())
			{
				m_cv.wait(lock);
			}
			task_t task(std::move(m_tasks.front()));
			m_tasks.pop_front();
			return task;
		}
		void loop()
		{
			for (;;)
			{
				task_t task = get_task();
				task();
			}
		}
	private:
		std::list<task_t> m_tasks;
		std::thread m_thread;
		mutex_t m_mutex;
		cond_t m_cv;
	};

///////////////////////////////////////////////////////////////////////////////////
	template<class T>
	class Dispatcher
	{
		static constexpr int THREAD_NUM = 8;
	public:
		Dispatcher() { start(); }
		~Dispatcher() { stop(); }
		ThreadWrap<T>* get_thread(int session_id)
		{
			int i = session_id % THREAD_NUM;
			return &m_threads[i];
		}
	private:
		void start()
		{
			for (int i=0;i< THREAD_NUM;i++)
			{
				m_threads[i].start();
			}
		}
		void stop()
		{
			for (int i = 0; i < THREAD_NUM; i++)
			{
				m_threads[i].stop();
			}
		}
	private:
		ThreadWrap<T> m_threads[THREAD_NUM];
	};
/////////////////////////////////////////////////////////////////////////////
	template<class T> using diaspatcher_t = Dispatcher<T>;
}