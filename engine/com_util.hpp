#pragma once
#include "engine_defines.hpp"
#include <ctime>

namespace Engine
{
	void localtime_r(const time_t* timep, struct tm* result);
	int get_cpu_cores();
	void set_resource();
}