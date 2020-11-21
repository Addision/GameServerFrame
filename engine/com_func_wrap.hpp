#pragma once
#include <functional>
#include <type_traits>

namespace Engine
{
	// 实现一个通用函数包装器
	template <typename R = void>
	class FuncWraper
	{
	public:
		// 接收函数对象的包装器
		template<class F, class... Args, class = typename std::enable_if<!std::is_member_function_pointer<F>::value>::type>
		void Wrap(F&& f, Args&&...args)
		{
			m_f = [&] { return f(args...); };
		}
		// 接受常量成员函数包装器
		template<class R, class C, class...DArgs, class P, class...Args>
		void Wrap(R(C::* f)(DArgs...) const, P&& p, Args&&... args)
		{
			m_f = [&, f] { return (*p.*f)(args...); };
		}
		// 接受非常量成员函数包装器
		template<class R, class C, class...DArgs, class P, class...Args>
		void Wrap(R(C::* f)(DArgs...), P&& p, Args&&... args)
		{
			m_f = [&, f] { return (*p.*f)(args...); };
		}

		R Excute()
		{
			return m_f();
		}
	private:
		std::function<R()> m_f;
	};
}

//int add(int a, int b)
//{
//	return a + b;
//}
//
//void testvoid(int a, int b)
//{
//	cout << "test void minus" << endl;
//}
//
//class FuncTest
//{
//public:
//	int add(int a, int b)
//	{
//		return a + b;
//	}
//
//	int minus(int a, int b) const
//	{
//		return a - b;
//	}
//};
//
//void SFTest::TestCommand()
//{
//	FuncTest functest;
//	FuncWraper<int> cmd;
//	cmd.Wrap(&FuncTest::add, &functest, 1, 2);
//	auto ret = cmd.Excute();
//	cout << "ret ==" << ret << endl;
//
//	cmd.Wrap(&FuncTest::minus, &functest, 2, 1);
//	ret = cmd.Excute();
//	cout << "ret ==" << ret << endl;
//
//	FuncWraper<void> cmd2;
//	cmd2.Wrap(testvoid, 1, 2);
//	cmd2.Excute();
//
//	cmd2.Wrap([](const std::string& str) { cout << str << endl; }, "abc");
//	cmd2.Excute();
//}