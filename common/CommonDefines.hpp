#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <cstddef>
#include <ctime>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <string>
#include <string.h>
#include <deque>
#include <queue>
#include <list>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <math.h>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <sstream>
#include <chrono>

using schar8 = std::int8_t;
using uchar8 = std::uint8_t;
using sint16 = std::int16_t;
using uint16 = std::uint16_t;
using sint32 = std::int32_t;
using uint32 = std::uint32_t;
using sint64 = std::int64_t;
using uint64 = std::uint64_t;

#include "engine/engine_defines.hpp"
#include "engine/logger.hpp"
#include "engine/mem_protobuf.hpp"

using namespace Engine;

static const char* SYSLOG_PATH = "../config/syslog/"; // 系统日志路径
static const char* SERVER_CONF = "../config/ServerConf.json";
