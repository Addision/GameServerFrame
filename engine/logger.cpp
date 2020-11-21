#include "logger.hpp"
#include <iostream>
#include <cstdarg>

namespace Engine
{
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#define CMD_COLOR(color,flag)							\
	inline std::ostream& color(std::ostream &s)				\
	{														\
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);	\
		SetConsoleTextAttribute(hStdout,					\
		flag | FOREGROUND_INTENSITY);						\
		return s;											\
	}

CMD_COLOR(green, FOREGROUND_GREEN | FOREGROUND_INTENSITY)
CMD_COLOR(blue, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
CMD_COLOR(yellow, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)
CMD_COLOR(red, FOREGROUND_RED | FOREGROUND_INTENSITY)
CMD_COLOR(normal, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#undef CMD_COLOR
#else
	static const char green[] = { 0x1b, '[', '1', ';', '3', '2', 'm', 0 };
	static const char red[] = { 0x1b, '[', '1', ';', '3', '1', 'm', 0 };
	static const char yellow[] = { 0x1b, '[', '1', ';', '3', '3', 'm', 0 };
	static const char blue[] = { 0x1b, '[', '1', ';', '3', '4', 'm', 0 };
	static const char normal[] = { 0x1b, '[', '0', ';', '3', '9', 'm', 0 };
#endif
///////////////////////////////////////////////////////////////////////////////////////////////
	ConsoleLog::ConsoleLog(LogLevel level):m_level(level)
	{
	}
	ConsoleLog::~ConsoleLog()
	{
	}

	void ConsoleLog::error(const char* log)
	{
		if (m_level < LogLevel::ERR) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		std::cout << "[" << red << "Error" << normal << "] " << log << std::endl;
	}

	void ConsoleLog::warn(const char* log)
	{
		if (m_level < LogLevel::WARN) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		std::cout << "[" << yellow << "Warn" << normal << "] " << log << std::endl;
	}

	void ConsoleLog::debug(const char* log)
	{
		if (m_level < LogLevel::DEBUG) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		std::cout << "[" << yellow << "Debug" << normal << "] " << log << std::endl;
	}

	void ConsoleLog::info(const char* log)
	{
		if (m_level < LogLevel::INFO) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		std::cout << "[" << green << "Info" << normal << "] " << log << std::endl;
	}

	void ConsoleLog::fatal(const char* log)
	{
		if (m_level < LogLevel::FATAL) return;
		std::unique_lock<std::mutex> lock(m_mutex);
		std::cout << "[" << red << "Fatal" << normal << "] " << log << std::endl;
	}
///////////////////////////////////////////////////////////////////////////////////////////////
	ILog* LogUtil::m_log = nullptr;
	LogLevel LogUtil::m_level = LogLevel::DEBUG;
	std::unique_ptr<LogUtil> g_pLog = std::make_unique<LogUtil>();

	void LogUtil::set_log(ILog* log)
	{
		if (!log) return;
		m_log = log;
		m_level = m_log->get_level();
	}

#define LOG_FORMAT(fmt)							\
	char str[MAX_LOG_LEN + 2];					\
	va_list arglist;							\
	va_start(arglist, fmt);						\
	vsnprintf(str, MAX_LOG_LEN, fmt, arglist);	\
	str[MAX_LOG_LEN] = 0x0;						\
	va_end(arglist);

#define LOG_OUTPUT(fmt, type)\
	LOG_FORMAT(fmt)\
	if(m_log) m_log->type(str)

	void LogUtil::debug(const char* fmt, ...)
	{
		LOG_OUTPUT(fmt, debug);
	}

	void LogUtil::info(const char* fmt, ...)
	{
		LOG_OUTPUT(fmt, info);
	}

	void LogUtil::warn(const char* fmt, ...)
	{
		LOG_OUTPUT(fmt, warn);
	}

	void LogUtil::error(const char* fmt, ...)
	{
		LOG_OUTPUT(fmt, error);
	}

	void LogUtil::fatal(const char* fmt, ...)
	{
		LOG_OUTPUT(fmt, fatal);
	}

	void LogUtil::fatal(std::int32_t no, const char* fmt, ...)
	{
		LOG_FORMAT(fmt);
		throw RunError(no, fmt);
	}

}
