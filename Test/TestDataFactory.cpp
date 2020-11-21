#include "GameTest.hpp"
#include "Engine/mem_data_factory.hpp"

using namespace Engine;

struct Data
{
	int a{ 0 };
};

void GameTest::TestDataFactory()
{
	MemData::DataFactory<Data, 10> kData;
	Data* p1 = kData.malloc();
	Data* p2 = kData.malloc();
	Data* p3 = kData.malloc();

	kData.free(p3);
	p3 = kData.malloc();
	kData.free(p2);

	MemData::DataPool<Data> kPool;
	kPool.init(2);
	Data* p4 = kPool.malloc();
	Data* p5 = kPool.malloc();
	Data* p6 = kPool.malloc();
	Data* p7 = kPool.malloc();
	Data* p8 = kPool.malloc();
	Data* p9 = kPool.malloc();
	Data* p10 = kPool.malloc();

	kPool.free(p4);
	kPool.free(p5);
	kPool.free(p6);
	kPool.free(p7);
	kPool.free(p8);
	kPool.free(p9);

	p4 = kPool.malloc();
	p5 = kPool.malloc();
	p6 = kPool.malloc();
	p7 = kPool.malloc();
	kPool.clear();
}
