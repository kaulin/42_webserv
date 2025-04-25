#include "Logger.hpp"

std::string Logger::getCurrentTime() const
{
	auto now = std::chrono::system_clock::now();
	std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

// constructor for a console logger
Logger::Logger() {}

// destructor
Logger::~Logger() {}

// overload << operator for stream manipulators (like std::endl)
Logger& Logger::operator<<(std::ostream& (*func)(std::ostream&))
{
	if (func == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
		_buffer << "\n";
	return *this;
}
