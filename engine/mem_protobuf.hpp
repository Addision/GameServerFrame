#pragma once
#include "google/protobuf/message.h"
#include "mem_buffer.hpp"
namespace Engine
{
	// Serialize to buffer
	class OutputStream : public ::google::protobuf::io::ZeroCopyOutputStream
	{
	public:
		OutputStream(IBuffer* buffer)
			:m_buffer(buffer){}
		virtual bool Next(void** data, int* size) override
		{
			int len = 0;
			if (last_size > 0)
			{
				m_buffer->commit_write(last_size);
			}
			*data = m_buffer->write(len);
			if (*data == nullptr) { return false; }
			last_size = len;
			*size = len;
			m_total_size += len;
			return true;
		}
		virtual void BackUp(int count) override
		{
			assert(last_size > 0);
			assert(count <= last_size && count > 0);
			m_total_size -= count;
			m_buffer->commit_write(last_size - count); // 最后一次写入
			last_size = 0;
		}
		virtual int64_t ByteCount() const override
		{
			return m_total_size;
		}
	private:
		IBuffer* m_buffer{nullptr};
		int m_total_size{0};
		int last_size{0}; // 每一次分配内存长度
	};
	// parse from buffer
	class InputStream : public ::google::protobuf::io::ZeroCopyInputStream
	{
	public:
		InputStream(IBuffer* buffer) :m_buffer(buffer) {}
		virtual bool Next(const void** data, int* size) override
		{
			int len = 0;
			*data = m_buffer->next(len);
			if (*data == nullptr) { return false; }
			last_size = len;
			*size = last_size;
			return true;
		}
		virtual void BackUp(int count) override
		{
			assert(last_size > 0);
			assert(count <= last_size && count > 0);
			m_buffer->back_up(count);
			last_size = 0;
		}
		virtual bool Skip(int count) override
		{
			last_size = 0; 
			return m_buffer->skip(count);
		}
		virtual int64_t ByteCount() const
		{
			return m_buffer->get_proto_read();
		}
	private:
		IBuffer* m_buffer{nullptr};
		int last_size{0};
	};
}