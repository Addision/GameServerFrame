#include "GameTest.hpp"

struct header
{
	std::int32_t m_id;
	std::int32_t m_len;
};

int main()
{
	GameTest::TestDataFactory();
	GameTest::TestLog();
	//GameTest::TestBuffer();
	//header h{ 1, 2 };
	//std::cout << sizeof(h) << std::endl;
	GameTest::TestProtoBuffer();
	GameTest::TestDispatcher();
#ifdef _WIN32
	system("pause");
#endif
	return 0;
}

