#pragma once
#include "net_session.hpp"
#include "mem_protobuf.hpp"

namespace Engine
{
	// Message Head[ MsgID(4) | MsgSize(4) ]
	struct MsgHeader
	{
		int msg_len;
		int msg_id;
	};

	static constexpr int MSG_HEADER_SIZE = sizeof(MsgHeader);

	enum class err : int
	{
		pack = -3,
		unpack = -2,
		pre_unpack = -1,
		none = 0,
		logic,
		null_handle,
	};

#define MSG_PROTOBUF_UNPACK(message,buffer)	\
	message k##message;{\
		InputStream kStream(buffer);\
		if(!k##message.ParseFromZeroCopyStream(&kStream))	\
			return -1;	\
	}

#define MSG_STREAM_UNPACK(message,buffer) \
	message k##message;{\
		buffer->reset();\
		int size = sizeof(message);\
		const char* p;\
		int len = size;\
		int pos = 0;\
		do{\
			p = msg->next(len);\
			if (!p) return msg::err::unpack;\
			memcpy((char*)(&k##message) + pos, p, len);\
			pos += len;\
			if (pos >= size) break;\
			len = size - pos;\
		} while (true);\
	}
}