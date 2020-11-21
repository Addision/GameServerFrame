#include "LogSystem.hpp"
#include "DateTime.hpp"

SysLog::SysLog()
{
}

SysLog::~SysLog()
{

}

char* SysLog::Write(int size)
{
	m_pWriterNode = m_kPool.malloc();
	m_pWriterNode->m_pReader = m_pWriterNode->m_pWriter = m_pWriterNode->m_buffer;
	return m_pWriterNode->m_pWriter;
}

void SysLog::CommitWrite(int size)
{
	m_pWriterNode->m_pWriter += size;
}

const char* SysLog::Read(int& size)
{
	size = (int)(m_pReaderNode->m_pWriter - m_pReaderNode->m_pReader);
	return m_pReaderNode->m_pReader;
}

void SysLog::CommitRead()
{
	node_t* tmpNode = m_pReaderNode;
	m_pReaderNode = m_pReaderNode->m_next;
	tmpNode->clear();
	m_kPool.free(tmpNode);
}

void SysLog::debug(const char* log)
{
	this->record(LogLevel::DEBUG, log);
}

void SysLog::info(const char* log)
{
	this->record(LogLevel::INFO, log);
}

void SysLog::warn(const char* log)
{
	this->record(LogLevel::WARN, log);
}

void SysLog::error(const char* log)
{
	this->record(LogLevel::ERR, log);
}

void SysLog::fatal(const char* log)
{
	this->record(LogLevel::FATAL, log);
}

void SysLog::Start(LogLevel level)
{
	m_state = state::starting;
	m_level = level;
	m_thread = std::thread(std::bind(&SysLog::save, this));
}

void SysLog::Stop()
{
	m_state = state::stopping;
	m_cv.notify_all();
	if (m_thread.joinable())
	{
		m_thread.join();
	}
}

void SysLog::record(LogLevel level, const char* str)
{
	if (m_level < level || str==nullptr) { return; }
	tm tmNow;
	time_t tNow = time(nullptr);
	Engine::localtime_r(&tNow, &tmNow);
	int len = (int)strlen(str);
	len += HEAD_LEN + 1;
	char* pBuffer = Write(len);
	snprintf(pBuffer, MAX_RECORD_LEN,
		"[%s][%04d-%02d-%02d %02d:%02d:%02d]%s\n", LogLevelStr[level-1],
		tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
		tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec,
		str);
	CommitWrite(len);
	std::unique_lock<std::mutex> locker(m_mutex);
	if (m_pReaderNode == nullptr)
	{
		m_pReaderNode = m_pWriterNode;
		m_pReaderTailNode = m_pReaderNode;
	}
	else
	{
		m_pReaderTailNode->m_next = m_pWriterNode;
		m_pReaderTailNode = m_pReaderTailNode->m_next;
	}
	m_cv.notify_one();
}

void SysLog::save()
{
	int len = 0;
	const char* p = nullptr;
	for (; m_state == state::starting;)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		while (!m_pReaderNode || m_state != state::starting)
		{
			m_cv.wait(locker);
		}
		if (m_pReaderNode == nullptr)
		{
			return;
		}
		p = Read(len);
		if (0 == len) {continue;}

		// log save
		std::cout << p;
		try
		{
			DateTime kDateTime;
			kDateTime.SetCurTime();
			std::string logFile = SYSLOG_PATH + std::string("log_") + kDateTime.YMDH();
			if (!m_fstream.is_open())
			{
				m_fstream.open(logFile, std::ios::out | std::ios::app);
			}
			if (m_fstream.good())
			{
				m_fstream << p << std::endl;
			}
		}
		catch (std::exception const&)
		{
			m_fstream.close();
			return;
		}
		CommitRead();
	}
}
