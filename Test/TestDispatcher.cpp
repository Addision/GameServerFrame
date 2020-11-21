#include "GameTest.hpp"
#include "engine/msg_dispatcher.hpp"

void test()
{
	std::cout << "aaaaaaaaaaa" << std::endl;
}

class TestFunc
{
public:
	void test()
	{
		std::cout << "bbbbbbbbbbbbbb" << std::endl;
	}
};

void GameTest::TestDispatcher()
{
	std::function<void()> functor = std::bind(test);
	
	diaspatcher_t<func_t> dispatcher;
	auto thread = dispatcher.get_thread(1212121);
	thread->add_task(std::move(functor));
	TestFunc t;
	thread->add_task(std::move(std::bind(&TestFunc::test, t)));
}