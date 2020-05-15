#include "Logger.h"

Logger::Logger(const std::string& logfile)
{
	file = fopen(logfile.c_str(), "wt");
}

Logger::~Logger()
{

	if (file != NULL)
	{
		while (!logs.empty())
		{
			LogInfo info;
			if (logs.try_pop(info)) {
				char tbuf[30];
				strftime(tbuf, 30, "%y %m %d %H:%M:%S", localtime(&info.time));
				fprintf(file, "%s %s\n", tbuf, info.log.c_str());
			}
		}
		fclose(file);
	}
}

void Logger::Log(const std::string& log)
{
	LogInfo info;
	info.log = log;
	info.time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	logs.push(info);
}

void Logger::WriteLog()
{
	LogInfo info;
	int cnt = 10;
	while (!logs.empty() && cnt > 0)
	{
		if (logs.try_pop(info)) {
			char tbuf[30];
			strftime(tbuf, 30, "%y %m %d %H:%M:%S", localtime(&info.time));
			fprintf(file, "%s %s\n", tbuf, info.log.c_str());
			cnt--;
		}
	}
}
