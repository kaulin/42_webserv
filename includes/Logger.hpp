#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>

class Logger
{
	private:
		static std::string getCurrentTime();
		Logger();
		~Logger();

	public:

		enum eStatus
		{
			OK = true,
			ERROR = false
		};

		static void log(eStatus ok, const std::string& message);
		static void start(const std::string& message);
		static void stop(const std::string& message);
};
