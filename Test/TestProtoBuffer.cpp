#include "GameTest.hpp"
#include "Engine/mem_stream_buffer.hpp"
#include "Engine/mem_protobuf.hpp"
#include "Game.pb.h"

using namespace Engine;

void GameTest::TestProtoBuffer()
{
	StreamBuffer msg_buffer;
	msg_buffer.init(RECV_BUFFER_SIZE);
	PbVector2 v2;
	v2.set_x(100);
	v2.set_y(200);
	OutputStream kStream(&msg_buffer);
	if (v2.SerializeToZeroCopyStream(&kStream))
	{
		PbVector2 kV2;
		InputStream kInstream(&msg_buffer);
		if (kV2.ParseFromZeroCopyStream(&kInstream))
		{
			std::cout << kV2.x() << std::endl;
			std::cout << kV2.y() << std::endl;
		}
	}
	else
	{
		std::cout << "write protobuf error" << std::endl;
	}
}
