#ifndef _MyEasyLog_h
#define _MyEasyLog_h

#include<ctime>
#include<iostream>
#include<sstream>
#include<fstream>
#include<direct.h>

#define LOG_FILE		"MyEasyLog.log"
#define LOG_ERROR		 1
#define LOG_WARNING		 2
#define LOG_NOTICE		 3

// [%Y-%m-%d %X level ]: title:\t log

class MyEasyLog
{
	static void writeCore(const std::string& level, const std::string& title, const std::string& log)
	{
		try
		{
			std::ofstream ofs;
			time_t t = time(0);
			char tmp[64];
			struct tm tm_obj;
			localtime_s(&tm_obj, &t);
			strftime(tmp, sizeof(tmp), "[%Y-%m-%d %X ", &tm_obj);
			ofs.open(LOG_FILE, std::ofstream::app);
			ofs << tmp << level << "]: ";
			ofs.write(title.c_str(), title.size());
			ofs << ":\t";
			ofs.write(log.c_str(), log.size());
			ofs << std::endl;
			ofs.close();
		}
		catch (...)
		{
			std::cerr << "WRITE LOG ERROE!\n";
		}
	}

public:
	static void write(int level, const std::string& title, const int& n)
	{
		std::ostringstream ostr;
		ostr << n;
		write(level, title, ostr.str());
	}
	static void write(int level, const std::string& title, const std::string& log)
	{
		switch (level)
		{
		case LOG_ERROR:
			writeCore("ERROR  ", title, log);
			break;
		case LOG_WARNING:
			writeCore("WARNING", title, log);
			break;
		case LOG_NOTICE:
#ifdef _DEBUG
			writeCore("NOTICE ", title, log);
#endif
			break;
		default:
			break;
		}
	
	}
};



#endif
