#pragma once

namespace Engine
{
	template<typename T>
	class Guard
	{
		using param_t = typename std::conditional<std::is_reference<T>::value || std::is_pointer<T>::value, T, T&>::type;
		using func_t = typename std::function<void(param_t)>;
	public:
		Guard(param_t param, func_t&& free_call):m_param(param),m_free_call(free_call){}
		Guard(param_t param, func_t&& init_call, func_t&& free_call)
			:m_param(param)
			,m_free_call(std::move(free_call))
		{
			init_call(param);
		}
		~Guard()
		{
			m_free_call(m_param);
		}
	private:
		param_t m_param;
		func_t m_free_call;
	};

	template<typename T>
	class AutoFree
	{
	public:
		AutoFree(T* t) :m_t(t) {}
		~AutoFree()
		{
			if (m_t)
			{
				delete m_t;
				m_t = nullptr;
			}
		}
	private:
		T* m_t;
	};
}
