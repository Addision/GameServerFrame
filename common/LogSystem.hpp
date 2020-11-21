#pragma once
#include "CommonDefines.hpp"
#include "engine/com_singleton.hpp"
#include "engine/mem_data_factory.hpp"

static const char LogLevelStr[][10] ={"Fatal", "ERR", "WARN", "INFO", "DEBUG"};

class SysLog : public ILog, public Singleton<SysLog>
{
	enum state {none=1, starting, stopping};
public:
	SysLog();
	~SysLog();
	virtual void debug(const char* log) override;
	virtual void info(const char* log) override;
	virtual void warn(const char* log) override;
	virtual void error(const char* log) override;
	virtual void fatal(const char* log) override;

	void Start(LogLevel level);
	void Stop();

public:
	// [DEBUG][YYYY-MM-DD HH:MM:SS] LOG_STR\n
	static constexpr int LEVEL_LEN = 7;
	static constexpr int DATE_LEN = 21;
	static constexpr int HEAD_LEN = LEVEL_LEN + DATE_LEN;
	static constexpr int BLOCK_SIZE = HEAD_LEN + MAX_LOG_LEN;
	static constexpr int MAX_RECORD_LEN = BLOCK_SIZE;
	struct node_t
	{
		node_t()
		{
			clear();
		}
		void clear()
		{
			memset(m_buffer, 0, MAX_RECORD_LEN);
			m_pWriter = m_pReader = m_buffer;
			m_pTail = m_buffer + MAX_RECORD_LEN;
			m_next = nullptr;
		}
		int left_len()
		{
			return (int)(m_pTail - m_pWriter);
		}
		char m_buffer[MAX_RECORD_LEN + 1];
		char* m_pWriter;
		char* m_pReader;
		char* m_pTail;
		struct node_t* m_next{nullptr};
	};
	char* Write(int size);
	void CommitWrite(int size);
	const char* Read(int& size);
	void CommitRead();
private:
	void record(LogLevel level, const char* str);
	void save();
private:
	std::thread m_thread;
	std::condition_variable m_cv;
	std::atomic<state> m_state = state::none;
	std::mutex m_mutex;
	std::ofstream m_fstream;

	MemData::DataPool<node_t, 5> m_kPool;
	node_t* m_pWriterNode{ nullptr };
	node_t* m_pReaderNode{ nullptr };
	node_t* m_pReaderTailNode{ nullptr };
};