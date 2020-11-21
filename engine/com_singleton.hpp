#pragma once
namespace Engine
{
	template<typename T>
	class Singleton
	{
	public:
		template<typename... Args>
		static T* Instance(Args&&... args)
		{
			static T instance{std::forward<Args>(args)...};
			return &instance;
		}
	protected:
		Singleton(void) = default;
		virtual ~Singleton(void) = default;
		Singleton(const Singleton&) = delete;
		Singleton& operator=(const Singleton&) = delete;
	};

	class NonCopyable
	{
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator = (const NonCopyable&) = delete;
	};
}
