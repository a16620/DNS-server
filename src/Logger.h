#pragma once
#include <concurrent_queue.h>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>

class Logger
{
	struct LogInfo {
		std::string log;
		std::time_t time;
	};
	concurrency::concurrent_queue<LogInfo> logs;
	FILE* file;
public:
	Logger(const std::string& logfile);
	~Logger();
	void Log(const std::string& log);
	void WriteLog();
};

