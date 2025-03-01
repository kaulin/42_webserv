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
		std::ostream*		_out;
		bool				_deleteStream;
		std::stringstream	_buffer;
		std::string			_filename;
		bool				_toFile;

		std::string getCurrentTime() const;

	public:
		// constructor for a console logger
		Logger(std::ostream& out = std::cout);

		// constructor for logging to file, uses fcntl to set fd to non-blocking
		Logger(const std::string& filename);

		// destructor
		~Logger();

		// overload << operator for stream manipulators (like std::endl)
		Logger& operator<<(std::ostream& (*func)(std::ostream&));

		void flushToFile();

		// function template for logging
		template <typename T>
		void log(const std::string& level, const T& message)
		{
			if (_toFile)
			{
				_buffer << "[" << getCurrentTime() << "] " 
					<< "[" << level << "] " 
					<< message << "\n";
			}
			else
			{
				std::cout << "[" << getCurrentTime() << "] " 
					<< "[" << level << "] " 
					<< message << "\n";
			}
		}

		// overload << operator for streaming
		template <typename T>
		Logger& operator<<(const T& message)
		{
			_buffer << message;
			return *this;
		}
};
