#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>

using mutex_t = std::mutex;
using guard_lock_t = std::lock_guard<std::mutex>;
using unique_lock_t = std::unique_lock<std::mutex>;
using cond_t = std::condition_variable;
using cond_any_t = std::condition_variable_any;

namespace Engine
{
	class AtomicLock
	{
	public:
		AtomicLock()
		{
			flag.clear();
		}
		~AtomicLock(){}
		void lock()
		{
			while (flag.test_and_set(std::memory_order_acquire));
		}
		bool trylock()
		{
			if (flag.test_and_set(std::memory_order_acquire))
			{
				return false;
			}
			return true;
		}
		void unlock()
		{
			flag.clear(std::memory_order_acquire);
		}
	private:
		mutable std::atomic_flag flag = ATOMIC_FLAG_INIT;
		AtomicLock(const AtomicLock&) = delete;
		AtomicLock& operator = (const AtomicLock&) = delete;
	};

}