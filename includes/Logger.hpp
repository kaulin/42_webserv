#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>

class Logger
{
	private:
		std::ostream*	_out;
		bool			_deleteStream;

		std::string		getCurrentTime() const {
							auto now = std::chrono::system_clock::now();
							std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
							std::stringstream ss;
							ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
							return ss.str();
						}
	public:
		// constructor for console logging
		Logger(std::ostream& out = std::cout) : _out(&out), _deleteStream(false) {}

		// constructor for logging to file
		Logger(const std::string& filename)
		{
			_out = new std::ofstream(filename, std::ios::app);
			_deleteStream = true;
			if (!*_out)
			{
				throw std::runtime_error("Failed to open log file.");
			}
		}
		~Logger() { if (_deleteStream) delete _out; }

		// basic logger template for different logging levels (INFO, WARNING, ERROR...)
		template <typename T>
		void log(const std::string& level, const T& message)
		{
			*_out << "[" << getCurrentTime() << "] " 
					<< "[" << level << "] " 
					<< message << std::endl;
		}
	
		// insertion operator overload for directly streaming something
		template <typename T>
		Logger& operator<<(const T& message)
		{
			*_out << message;
			return *this;
		}
	
		// template for stream manipulators like endl
		Logger& operator<<(std::ostream& (*func)(std::ostream&))
		{
			*_out << func;
			return *this;
		}
};