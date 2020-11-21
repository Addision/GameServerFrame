#pragma once
#include <stdint.h>
#include <list>

namespace Engine
{
	namespace MemBuffer
	{
		template<int32_t N>
		class Buffer
		{
		public:
			Buffer() :buf_len(N), read_pos(0), write_pos(0) {}
			void clear()
			{
				memset(buf, 0, buf_len);
				read_pos = write_pos = 0;
			}
			int32_t data_len() { return write_pos - read_pos; }
			int32_t left_len() { return buf_len - write_pos; }
			bool empty() { return write_pos == 0 || write_pos == read_pos; }

			int32_t read_pos{ 0 }; // 开始读数据位置
			int32_t write_pos{ 0 };	// 开始写数据位置
			int32_t buf_len{ 0 };
			char buf[N];
		};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<class Buffer>
		class Allocate   // 缓存没有使用的buffer
		{
		public:
			using buffer_list_t = std::list<Buffer*>;
		public:
			Allocate() {}
			~Allocate() { clear(); }
			Buffer* malloc()
			{
				Buffer* p = nullptr;
				if (!buffer_list.empty())
				{
					p = buffer_list.front();
					buffer_list.pop_front();
				}
				else
				{
					p = new Buffer;
				}
				p->clear();
				return p;
			}
			void free(Buffer* p)
			{
				buffer_list.emplace_back(p);
			}
			void clear()
			{
				for (auto it = buffer_list.begin(); it != buffer_list.end(); it++)
				{
					delete (*it);
				}
				buffer_list.clear();
			}
		private:
			buffer_list_t buffer_list;  // cache empty buffer
		};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<int32_t N, int32_t G=1> // N buffer len G buffer count
		class BufferPool
		{
			using buffer_t = typename Buffer<N>;
			using allocator_t = typename Allocate<buffer_t>;
			using buffer_list_t = typename Allocate<buffer_t>::buffer_list_t;
		public:
			BufferPool() { init(); }
			~BufferPool() { clear(); }
			// read | write 做为普通缓存操作接口
			void write(const char* data, int32_t size)
			{
				buffer_t* buffer = nullptr;
				int32_t write_pos = 0;
				int32_t left = 0;
				while (size>0)
				{
					buffer = get_write_buffer();
					left = buffer->left_len();
					if (size > left)
					{
						memcpy(buffer->buf + buffer->write_pos, data + write_pos, left);
						write_pos += left;
						size -= left;
						buffer->write_pos += left;
					}
					else
					{
						memcpy(buffer->buf+ buffer->write_pos, data + write_pos, size);
						buffer->write_pos += size;
						size = 0;
					}
				}
			}
			void read(char* buf, int32_t size)
			{
				buffer_t* buffer = nullptr;
				int32_t read_pos = 0;
				int32_t len = 0;
				while (size>0)
				{
					buffer = get_read_buffer();
					if (buffer == nullptr) return;
					len = buffer->data_len();
					if (size > len)
					{
						memcpy(buf + buffer->read_pos, buffer->buf, len);
						read_pos += len;
						size -= len;
						buffer->read_pos += len;
					}
					else
					{
						memcpy(buf + buffer->read_pos, buffer->buf, size);
						buffer->read_pos += size;
						size = 0;
					}
					free(buffer);
				}
			}
			// 以下为socket接收发送操作接口
			char* get_recv_buffer(int32_t& size)
			{
				Buffer* buffer = get_write_buffer();
				size = buffer->left_len();
				return buffer->buf;
			}
			char* get_send_buffer(int32_t& size)
			{
				Buffer* buffer = get_read_buffer();
				size = buffer->data_len();
				return buffer->buf;
			}
			void post_recv_size(int32_t size)
			{
				Buffer* buffer = m_buffer_list.front();
				buffer->write_pos += size;
			}
			void post_send_size(int32_t size)
			{
				Buffer* buffer = m_buffer_list.back();
				buffer->read_pos += size;
				free(buffer);
			}
//////////////////////////////////////////////////////////////////////////////
			void init()
			{
				for (size_t i=0;i<G;i++)
				{
					buffer_t* p = new buffer_t;
					m_allocator.free(p);
				}
			}
			void init(int count)
			{
				for (size_t i = 0; i < count; i++)
				{
					buffer_t* p = new buffer_t;
					m_allocator.free(p);
				}
			}
			void clear()  // free buffer resource
			{
				m_allocator.clear();
				for (auto it = m_buffer_list.begin(); it != m_buffer_list.end(); it++)
				{
					delete (*it);
				}
				m_buffer_list.clear();
			}
		private:
			buffer_t* get_write_buffer()
			{
				buffer_t* buffer = nullptr;
				if (!m_buffer_list.empty())
				{
					buffer = m_buffer_list.front();
				}
				if (!buffer || buffer->left_len() == 0)
				{
					buffer = m_allocator.malloc();
					m_buffer_list.emplace_front(buffer);
				}
				return buffer;
			}
			buffer_t* get_read_buffer()
			{
				buffer_t* buffer = nullptr;
				if (m_buffer_list.empty())
				{
					return nullptr;
				}
				return m_buffer_list.back();
			}
			void free(buffer_t* buffer)
			{
				if (buffer || buffer->empty())
				{
					m_allocator.free(buffer);
					m_buffer_list.pop_back();
				}
			}
			int32_t total_len()
			{
				int32_t len = 0;
				for (auto it : m_buffer_list)
				{
					len += it->data_len();
				}
				return len;
			}
		private:
			allocator_t m_allocator;
			buffer_list_t m_buffer_list;  // 保存使用的buffer
		};
////////////////////////////////////////////////////////////////////////////////////////////////////////
		template<int32_t N>
		using buffer_pool_t = BufferPool<N>;
	}
}
