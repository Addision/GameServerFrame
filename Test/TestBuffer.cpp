#include "GameTest.hpp"
#include "Engine/mem_buffer_factory.hpp"

using namespace Engine;

void GameTest::TestBuffer()
{
	MemBuffer::buffer_pool_t<50> kBufferPool;
	kBufferPool.init(10);
	kBufferPool.write("1234567890", 10);
	kBufferPool.write("1234567890", 10);
	kBufferPool.write("1234567890", 10);
	kBufferPool.write("1234567890", 10);
	char buff[1000];
	memset(buff, 0, 1000);
	kBufferPool.read(buff, 40);
	std::cout << buff << std::endl;
}