#include "Logger.hpp"

Logger::Logger() {}
Logger::~Logger() {}

void Logger::log(eStatus ok, const std::string& message)
{
	std::cout << (ok ? "" : "[ERROR] ") 
		<< "[" << getCurrentTime() << "] " 
		<< message << "\n";
}

std::string Logger::getCurrentTime()
{
	auto now = std::chrono::system_clock::now();
	std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}