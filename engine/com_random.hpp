#pragma once
#include "com_singleton.hpp"
#include <random>

namespace Engine
{
	class Random : public Singleton<Random>
	{
	public:
		Random()
		{
			m_gen = std::mt19937(m_rd());
		}
		~Random() {}

		int32_t rand()
		{
			std::uniform_int_distribution<> dis;
			return dis(m_gen);
		}

		int32_t rand(int32_t min, int32_t max)
		{
			if (max <= min) max = min;
			std::uniform_int_distribution<> dis(min, max);
			return dis(m_gen);
		}

		template<typename T>
		T rand(T min, T max)
		{
			if (max <= min) max = min;
			std::uniform_real_distribution<T> dis(min, max);
			return dis(m_gen);
		}
	private:
		std::mt19937 m_gen;  // 利用梅森旋转伪随机数生成器
		std::random_device m_rd;  //get seed  
	};
}

//	int32_t n1 = Random::GetInstance()->rand(1, 10);
//	cout << n1 << endl;
//	float n2 = Random::GetInstance()->rand<float>(1, 10);
//	cout << n2 << endl;
//	double n3 = Random::GetInstance()->rand<double>(1, 10);
//	cout << n3 << endl;
//	return 0;