#pragma once
#include "CommonDefines.hpp"
#include "engine/com_util.hpp"

class DateTime
{
public:
	void SetCurTime()
	{
		struct tm _tm;
		time_t t = time(0);
		localtime_r(&t, &_tm);

		m_year = _tm.tm_year + 1900;
		m_mon = _tm.tm_mon + 1;
		m_day = _tm.tm_mday;
		m_hour = _tm.tm_hour;
		m_min = _tm.tm_min;
		m_sec = _tm.tm_sec;
	}

	time_t GetTime()
	{
		struct tm _tm;
		_tm.tm_year = m_year - 1900;
		_tm.tm_mon = m_mon - 1;
		_tm.tm_mday = m_day;
		_tm.tm_hour = m_hour;
		_tm.tm_min = m_min;
		_tm.tm_sec = m_sec;
		return mktime(&_tm);
	}

	std::string GetStrTime()
	{
		struct tm _tm;
		time_t t = time(0);
		localtime_r(&t, &_tm);
		char str[50];
		memset(str, 0, 50);
		strftime(str, 50, "%F %T", &_tm);
		return str;
	}

	std::string YMDHMS()
	{
		char buf[50];
		snprintf(buf, 50, "%d%d%d%d%d%d", m_year, m_mon, m_day, m_hour, m_min, m_sec);
		return buf;
	}

	std::string YMDH()
	{
		char buf[50];
		snprintf(buf, 50, "%d%d%d%d", m_year, m_mon, m_day, m_hour);
		return buf;
	}

	sint32 m_year;
	sint32 m_mon;
	sint32 m_day;
	sint32 m_hour;
	sint32 m_min;
	sint32 m_sec;
};