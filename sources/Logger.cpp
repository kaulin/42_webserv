#include "webserv.hpp"

std::string Logger::getCurrentTime() const
{
	auto now = std::chrono::system_clock::now();
	std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

// constructor for a console logger
Logger::Logger(std::ostream& out) : _out(&out), _deleteStream(false), _toFile(false) {}

// constructor for logging to file, uses fcntl to set fd to non-blocking
Logger::Logger(const std::string& filename) : _filename(filename), _toFile(true)
{
	_out = new std::ofstream(filename, std::ios::app);
	_deleteStream = true;

	if (!static_cast<std::ofstream*>(_out)->is_open())
		throw std::runtime_error("Failed to open log file.");
}

// destructor
Logger::~Logger()
{
	if (_toFile)
		flushToFile();
	
	if (_deleteStream)
		delete _out;
}

void Logger::flushToFile()
{
	if (_toFile)
	{
		std::ofstream file(_filename, std::ios::app);
		if (!file.is_open())
			throw std::runtime_error("Failed to write to log.");
		file << _buffer.str();
		file.close();
		_buffer.str("");
		_buffer.clear();
	}
}

// overload << operator for stream manipulators (like std::endl)
Logger& Logger::operator<<(std::ostream& (*func)(std::ostream&))
{
	if (func == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
		_buffer << "\n";
	return *this;
}
