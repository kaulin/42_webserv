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
		std::stringstream	_buffer;

		std::string getCurrentTime() const;

	public:
		// constructor for a console logger
		Logger();

		// destructor
		~Logger();

		// overload << operator for stream manipulators (like std::endl)
		Logger& operator<<(std::ostream& (*func)(std::ostream&));

		// function template for logging
		template <typename T>
		void log(const std::string& level, const T& message)
		{
			std::cout << "[" << getCurrentTime() << "] " 
				<< "[" << level << "] " 
				<< message << "\n";
		}

		// overload << operator for streaming
		template <typename T>
		Logger& operator<<(const T& message)
		{
			_buffer << message;
			return *this;
		}
};
