#pragma once
#include <sstream>
#include <stdexcept>
#include <exception>
#include <memory>
#include <mutex>

namespace Engine
{
	static constexpr int MAX_LOG_LEN = 1024;

	enum LogLevel
	{
		FATAL = 1,
		ERR = 2,
		WARN = 3,
		INFO = 4,
		DEBUG = 5,
	};
	enum LogErrors
	{

	};
	enum EngineErrs
	{
		E_DEFAULT = 0,
		E_NET,
	};
	// 自定义异常
	class RunError : public std::runtime_error
	{
	public:
		RunError(std::int32_t no, const char* what):std::runtime_error(what?what:""),m_err_no(no){}
		~RunError() = default;
	private:
		std::int32_t m_err_no;
	};
	// log 日志接口
	class ILog
	{
	public:
		virtual void debug(const char*) = 0;
		virtual void info(const char*) = 0;
		virtual void warn(const char*) = 0;
		virtual void error(const char*) = 0;
		virtual void fatal(const char*) = 0;
		LogLevel get_level() { return m_level; }
	protected:
		LogLevel m_level;
	};
//////////////////////////////////////////////////////////////////////////////////
	class ConsoleLog : public ILog
	{
	public:
		ConsoleLog(LogLevel level);
		ConsoleLog() :m_level(LogLevel::DEBUG) {}
		~ConsoleLog();
		virtual void debug(const char* log) override;
		virtual void info(const char* log) override;
		virtual void warn(const char* log) override;
		virtual void error(const char* log) override;
		virtual void fatal(const char* log) override;
	private:
		LogLevel m_level;
		std::mutex m_mutex;
	};
/////////////////////////////////////////////////////////////////////////////////////
#define LOG_STREAMOUT(type)\
	if(m_log) {m_log->type(m_oss.str().c_str());}

	class LogUtil   // 日志记录辅助接口
	{
	public:
		LogUtil() {}
		~LogUtil() {}
		static void set_log(ILog* log); // 设置log保存日志方式
		static void debug(const char* fmt, ...);
		static void info(const char* fmt, ...);
		static void warn(const char* fmt, ...);
		static void error(const char* fmt, ...);
		static void fatal(const char* fmt, ...);
		static void fatal(std::int32_t no, const char* fmt, ...);

		template<typename T>
		LogUtil& operator<<(const T& log)
		{
			m_oss << log;
			return *this;
		}
		LogUtil& operator<<(std::ostream& (*log)(std::ostream&)) // << std::endl
		{
			switch (m_level)
			{
			case LogLevel::DEBUG:
				LOG_STREAMOUT(debug);
				break;
			case LogLevel::INFO:
				LOG_STREAMOUT(info);
				break;
			case LogLevel::WARN:
				LOG_STREAMOUT(warn);
				break;
			case LogLevel::ERR:
				LOG_STREAMOUT(error);
				throw RunError(4, "program exit");
				break;
			case LogLevel::FATAL:
				LOG_STREAMOUT(fatal);
				throw RunError(5, "program exit");
				break;
			default:
				break;
			}
			m_oss.str("");
			return *this;
		}
		LogUtil& Stream(LogLevel level) { m_level = level; return *this; }

		LogUtil(const LogUtil&) = delete;
		LogUtil& operator=(const LogUtil&) = delete;
		LogUtil(const LogUtil&&) = delete;
		LogUtil& operator=(const LogUtil&&) = delete;
	private:
		static ILog* m_log;
		std::ostringstream m_oss;
		// for stream log
		static LogLevel m_level;
	};

	extern std::unique_ptr<LogUtil> g_pLog;

#define LogDebug	g_pLog->Stream(LogLevel::DEBUG)
#define LogInfo		g_pLog->Stream(LogLevel::INFO)
#define LogWarn		g_pLog->Stream(LogLevel::WARN)
#define LogErr		g_pLog->Stream(LogLevel::ERR)
#define LogFatal	g_pLog->Stream(LogLevel::FATAL)
#define LogEnd		std::endl;

}




