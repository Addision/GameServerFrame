#pragma once
#include "com_lock.hpp"
#include <functional>
#include <future>
#include <thread>
#include <list>
namespace Engine
{
	using func_t = std::function<void()>;
	template<class T>
	class Task : public T
	{
	public:
		Task(){}
		Task(func_t&& func):m_func(std::move(func)) {}
		Task(T&& task):T(std::move(task)){}
		void operator()()
		{
			m_func ? m_func() : T::run();
		}
	private:
		func_t m_func;
	};
///////////////////////////////////////////////////////////////////////////////////////
	template<class T>
	class ThreadPoolWrap
	{
		using task_t = typename std::conditional<std::is_same<func_t, T>::value, T, Task<T>>::type;
	public:
		enum class state { none, running, stopping };
		bool add_task(task_t&& task)
		{
			unique_lock_t lock(m_mutex);
			if (m_state != state::running)
			{
				return false;
			}
			m_tasks.emplace_back(std::move(task));
			++m_ntask;
			m_cv.notify_all();
			return true;
		}
		template<class F, typename... Args>
		bool add_task(F&& f, Args&&... args)
		{
			return add_task(func_t(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
		}
		template<class F, typename... Args>
		decltype(auto) commit_future(F&& f, Args&&... args)
		{
			using ResType = decltype(std::bind(std::forward<F>(f), std::forward<Args>(args)...)());
			std::future<ResType> future;
			auto task = std::make_shared<std::packaged_task<ResType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
				);
			if (commit([task]() { (*task)(); }))
			{
				future = task->get_future();
			}
			return future;
		}
		void quit()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_state != state::running) { return; }

			m_state = state::stopping;
			if (m_tasks.empty())
			{
				lock.unlock();
				destroy();
				return;
			}

			auto task = std::make_shared<std::packaged_task<bool()>>([]() { return true; });
			m_tasks.emplace_back([task]() { (*task)(); });

			lock.unlock();

			++m_ntask;
			m_cv.notify_one();

			task->get_future().get();
			destroy();
		}
		size_t size() { return m_size; }
		size_t task_size() { return m_ntask; }
		task_t get_task()
		{
			unique_lock_t lock(m_mutex);
			while (m_tasks.empty())
			{
				m_cv.wait(lock);
			}
			if (m_state != state::running)
			{
				return task_t();
			}

			task_t task(std::move(m_tasks.front()));
			m_tasks.pop_front();
			--m_ntask;
			return task;
		}
	private:
		void loop()
		{
			for (;;)
			{
				task_t task = get_task();
				task();
			}
		}
	protected:
		void init()
		{
			for (size_t i=0;i<m_size;++i)
			{
				m_threads[i] = std::thread(&ThreadPoolWrap::loop, this);
			}
			m_state = state::running;
		}
		void destroy()
		{
			unique_lock_t lock(m_mutex);
			m_state = state::stopping;
			m_tasks.clear();
			lock.unlock();
			m_cv.notify_all();
			for (size_t i=0;i<m_size;++i)
			{
				if (m_threads[i].joinable())
				{
					m_threads[i].join();
				}
			}
		}
	protected:
		std::thread* m_threads{nullptr};
		std::list<task_t> m_tasks;
		std::atomic<state> m_state = state::none;
		mutex_t m_mutex;
		cond_t m_cv;
		size_t m_size = 0;
		std::atomic_size_t m_ntask;
	};
//////////////////////////////////////////////////////////////////////////////////////
	template<size_t N, typename T = std::function<void()>>
	class ThreadPool : public ThreadPoolWrap<T>
	{
		using base_t = ThreadPoolWrap<T>;
	public:
		ThreadPool()
		{
			this->m_threads = _threads;
			this->m_size = N;
			this->m_state = base_t::state::none;
			this->init();
		}
		~ThreadPool()
		{
			this->quit();
			this->m_threads = nullptr;
		}
	private:
		std::thread _threads[N];
	};
//////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class ThreadPool<0, T> : public ThreadPoolWrap<T>
	{
		using base = ThreadPoolWrap < T >;
	public:
		ThreadPool<0, T>(){}
		ThreadPool<0, T>(size_t size)
		{
			this->init(size);
		}
		~ThreadPool<0, T>()
		{
			this->base::quit();
			delete[] this->m_threads;
			this->m_threads = nullptr;
		}
		void init(size_t size)
		{
			this->m_size = size;
			this->m_state = base::state::none;
			this->m_threads = new std::thread[size];
			this->base::init();
		}
	};
////////////////////////////////////////////////////////////////////////////////////////////////////////
	using thread_t = ThreadPool<1>;
	template<size_t N> using thread_pool_t = ThreadPool<N>;
	template<class T> using taskpool_t = ThreadPool < 0, T >;
	template<size_t N, class T> using taskpools_t = ThreadPool < N, T >;
}