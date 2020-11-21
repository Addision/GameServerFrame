#pragma once

#include "engine_defines.hpp"
#include "mem_data_factory.hpp"
#include "mem_buffer.hpp"
#include "com_lock.hpp"

namespace Engine
{
	template<int32_t N>
	struct BufferNode
	{
		void init()
		{
			memset(m_buffer, 0, sizeof(m_buffer));
			m_reader = m_writer = m_buffer;
			next_node = nullptr;
		}
		char m_buffer[N + 1];
		BufferNode* next_node{nullptr};
		char* m_reader{ nullptr };
		char* m_writer{ nullptr };
	};
	//////////////////////////////////////////////////////////////////////////////////////////
	class StreamBuffer : public IBuffer
	{
	public:
		StreamBuffer(void);
		virtual ~StreamBuffer(void);

		virtual void init(int size) override;
		virtual void clear() override;
		virtual char* read(int& size) override;
		virtual bool commit_read(int size) override;
		virtual char* write(int& size) override;
		virtual void commit_write(int size) override;
		virtual int readable_size() override;
		virtual int writable_size() override;
	public:
		virtual const char* next(int& size) override;
		virtual bool skip(int size) override;
		virtual bool back_up(int size) override;
		virtual void reset() override;
	public:
		int get_msg_id();
		int get_msg_len();
	private:
		int parse_msg_head();
		const char* _next(int& size, int limit);
	public:
		using stream_node_t = typename BufferNode<BLOCK_SIZE>;
		static constexpr int block_size = BLOCK_SIZE;
		using factory_t = typename MemData::DataPool<stream_node_t>;
	private:
		factory_t m_factory;
		stream_node_t* m_head{ nullptr };
		stream_node_t* m_tail{ nullptr };
		stream_node_t* m_next{nullptr};

		int m_readable{ 0 };
		int m_offset{ 0 };
		std::mutex m_mutex;
	};

}
