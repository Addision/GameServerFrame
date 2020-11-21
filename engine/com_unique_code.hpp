#pragma once
#include <cstdint>
#include <ctime>
namespace Engine
{
	class UniqCode
	{
	public:
		UniqCode(std::uint16_t key) : m_time(time(nullptr)), m_index(0), m_key(key) { m_key <<= 16; }
		~UniqCode(void) = default;

		std::uint64_t gen_code(void) {
			std::uint64_t code = m_time;
			code = code << 32;
			code |= m_key;
			code |= m_index;
			if (++m_index > 0xFFFF)
			{
				m_index = 0;
				++m_time;
			}
			return code;
		}
		time_t get_seed(void) { return m_time; }
	private:
		time_t m_time;
		std::uint32_t m_index;
		std::uint32_t m_key;
	};
}