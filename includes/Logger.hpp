#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>

template <typename StreamType>
class Logger
{
	private:
		StreamType*	_out;
		bool		_deleteStream;

		std::string getCurrentTime() const
		{
			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
			return ss.str();
		}

		// // sets file descriptor status flag to non-blocking
		// void setNonBlocking(int fd)
		// {
		// 	int flags = fcntl(fd, F_GETFL, 0);
		// 	if (flags != -1)
		// 	{
		// 		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		// 	}
		// }

	public:
		// constructor for a console logger
		Logger(StreamType& out = std::cout) : _out(&out), _deleteStream(false) {}

		// constructor for logging to file, uses fcntl to set fd to non-blocking
		Logger(const std::string& filename)
		{
			int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd == -1)
			{
				throw std::runtime_error("Failed to open log file.");
			}

			fcntl(fd, F_SETFL, O_NONBLOCK);

			_out = new std::ofstream(filename, std::ios::app);
			_deleteStream = true;
		}

		// destructor
		~Logger() { if (_deleteStream) delete _out; }

		// function template for logging
		template <typename T>
		void log(const std::string& level, const T& message)
		{
			*_out << "[" << getCurrentTime() << "] " 
				  << "[" << level << "] " 
				  << message << std::endl;
		}

		// overload << operator for streaming
		template <typename T>
		Logger& operator<<(const T& message)
		{
			*_out << message;
			return *this;
		}

		// overload << operator for stream manipulators (like std::endl)
		Logger& operator<<(std::ostream& (*func)(std::ostream&))
		{
			*_out << func;
			return *this;
		}
};
