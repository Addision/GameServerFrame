#pragma once
#include <stdint.h>
#include <list>
#include <vector>
#include <mutex>

namespace Engine
{
	namespace MemData
	{
		template<class T, size_t N>
		class DataFactoryWrap
		{
		public:
			T* malloc()
			{
				if (m_left == 0) return nullptr;
				int32_t next = m_next;
				T* data = get_data(next);
				if (data) return data;
				while (m_offset[++next] && next < (m_size-1));
				data = get_data(next);
				if (data) return data;
				next = m_next;
				while (m_offset[--next] && next > 0);
				data = get_data(next);
				m_next = next + 1;
				if (m_next == m_size) m_next = 0;
				return data;
			}
			void free(T* p)
			{
				if (!p) return;
				int32_t pos = npos(p);
				if (pos == -1) return;
				if (m_offset[pos] == 0) return;
				m_offset[pos] = 0;
				m_next = pos;
				m_left++;
			}
			void clear()
			{
				delete[] m_data;
				delete[] m_offset;
				m_data = nullptr;
				m_offset = nullptr;
				m_size = 0;
				m_left = 0;
				m_next = 0;
			}
			int32_t npos(T* p)
			{
				if (!m_data || p < m_data || p >= m_data + m_size) return -1;
				return (int32_t)(p - m_data);
			}
			int32_t size() { return m_size; }
			int32_t left() { return m_left; }
			int32_t used() { return m_size - m_left; }
		private:
			T* get_data(int32_t next)
			{
				if (m_offset[next] == 0)
				{
					m_offset[next] = 1;
					m_left--;
					return m_data + next;
				}
				return nullptr;
			}
		protected:
			T* m_data{ nullptr };
			int32_t* m_offset{ nullptr };
			int32_t m_size, m_next, m_left;
		};

		template<class T, size_t N>
		class DataFactory : public DataFactoryWrap<T, N>
		{
		public:
			DataFactory()
			{
				this->m_data = new T[N];
				this->m_offset = new int32_t[N];
				std::fill_n(this->m_offset, N, 0);
				this->m_size = N;
				this->m_left = N;
				this->m_next = 0;
			}
			~DataFactory()
			{
				this->clear();
			}

			void init(int32_t size)
			{

			}
		};

		template<class T>
		class DataFactory<T, 0> :public DataFactoryWrap<T, 0>
		{
		public:
			DataFactory<T, 0>()
			{
			}
			~DataFactory<T, 0>()
			{
				this->clear();
			}
			void init(int32_t size)
			{
				this->m_data = new T[size];
				this->m_offset = new int32_t[size];
				std::fill_n(this->m_offset, size, 0);
				this->m_size = size;
				this->m_left = size;
				this->m_next = 0;
			}
		};
		///////////////////////////////////////////////////////////////////////////////
		template<class Factory>
		class Allocator
		{
		public:
			Factory* malloc(int32_t size)
			{
				Factory* p = nullptr;
				if (!m_cache.empty())
				{
					p = m_cache.front();
					m_cache.pop_front();
				}
				else
				{
					p = new Factory;
					p->init(size);
				}
				return p;
			}
			void free(Factory* p)
			{
				if (!p) return;
				m_cache.emplace_back(p);
			}
			void clear()
			{
				for (auto it : m_cache)
				{
					delete it;
				}
				m_cache.clear();
			}
		private:
			std::list<Factory*> m_cache;
		};
		/////////////////////////////////////////////////////////////////////////////////////
		template<class T, int32_t N = 0, int32_t G = 0>
		class DataPool
		{
			using base_t = typename DataFactory<T, G>;
			using main_t = typename DataFactory<T, N>;
			using chunks_t = typename std::vector<base_t*>;
			using allocator_t = typename Allocator < DataFactory<T, G> >;
		public:
			DataPool()
			{
				if (N > 0)
				{
					m_main_chunk.init(N);
				}
				m_grow = G;
				m_next = 0;
				m_free = 0;
			}
			~DataPool()
			{
				this->clear();
			}
			void init(int32_t init_size = 0, int32_t grow_size = 0)
			{
				m_main_chunk.init(init_size);
				if (grow_size == 0) 
					m_grow = init_size;
				else
					m_grow = grow_size;
			}
			void clear()
			{
				m_main_chunk.clear();
				for (auto it = m_chunk_vec.begin(); it != m_chunk_vec.end(); it++)
				{
					(*it)->clear();
				}
				m_chunk_vec.clear();
				m_next = 0;
				m_free = 0;
			}
			T* malloc()
			{
				std::lock_guard<std::mutex> locker(m_mutex);
				T* p  = m_main_chunk.malloc();
				if (p) { return p; }
				if (!m_chunk_vec.empty())
				{
					p = m_chunk_vec[m_next]->malloc();
				}
				if (!p)
				{
					base_t* p_factory = m_allocator.malloc(m_grow);
					m_chunk_vec.emplace_back(p_factory);
					m_next = (int32_t)m_chunk_vec.size() - 1;
					return p_factory->malloc();
				}
				return p;
			}
			void free(T* p)
			{
				std::lock_guard<std::mutex> locker(m_mutex);
				int32_t pos = m_main_chunk.npos(p);
				if (pos != -1)
				{
					m_main_chunk.free(p);
					return;
				}
				if (m_chunk_vec.empty()) { return; }
				for (int32_t i = 0; i < m_next; i++)
				{
					int32_t pos = m_chunk_vec[i]->npos(p);
					if (pos == -1) 
						continue;
					else
					{
						m_chunk_vec[i]->free(p);
						m_free = i;
						break;
					}
				}
				if (m_chunk_vec[m_free]->used() == 0)
				{
					base_t* factory = m_chunk_vec[m_free];
					m_chunk_vec[m_free] = m_chunk_vec[m_next];
					m_allocator.free(factory);
					m_chunk_vec.pop_back();
					m_next--;
				}
			}
		private:
			main_t m_main_chunk;
			chunks_t m_chunk_vec;
			int32_t m_grow;
			allocator_t m_allocator;
			int32_t m_next{ 0 }, m_free{0};
			std::mutex m_mutex;

			DataPool(const DataPool&) = delete;
			DataPool& operator=(const DataPool&) = delete;
		};


		/////////////////////////////////////////////////////////////////////////////////
		template<class T, std::uint64_t N>
		using DataPoolEx = DataPool<T, N, N>;
	}
}