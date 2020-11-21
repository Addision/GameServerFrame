#pragma once

namespace Engine
{
	class IMsgBuffer
	{
	public:
		IMsgBuffer() {}
		virtual ~IMsgBuffer() = default;
		virtual const char* next(int& size) = 0;
		virtual bool skip(int size) = 0;
		virtual bool back_up(int size) = 0;
		virtual void reset() = 0;
		int get_proto_read() { return m_proto_read; }
		int get_proto_size() { return m_proto_size; }
		void set_proto_size(int size) { m_proto_size = size; }
	protected:
		int m_proto_read{0};  // 读取每条协议当前长度 
		int m_proto_size{0}; // 每条协议大小
	};

	class IBuffer : public IMsgBuffer
	{
	public:
		virtual void init(int size) = 0;
		virtual void clear() = 0;
		virtual char* read(int& size) = 0;
		virtual bool commit_read(int size) = 0;
		virtual char* write(int& size) = 0;
		virtual void commit_write(int size) = 0;
		virtual int readable_size() = 0;
		virtual int writable_size() = 0;
		virtual int get_msg_id() = 0;
		virtual int get_msg_len() = 0;
	};


}

