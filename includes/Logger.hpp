#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>

template <typename StreamType>
class Logger
{
	private:
		StreamType*			_out;
		bool				_deleteStream;
		std::stringstream	_buffer;
		std::string			_filename;
		bool				_toFile;

		std::string getCurrentTime() const
		{
			auto now = std::chrono::system_clock::now();
			std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S");
			return ss.str();
		}

	public:
		// constructor for a console logger
		Logger(StreamType& out = std::cout) : _out(&out), _deleteStream(false), _toFile(false) {}

		// constructor for logging to file, uses fcntl to set fd to non-blocking
		Logger(const std::string& filename) : _filename(filename), _toFile(true)
		{
			_out = new std::ofstream(filename, std::ios::app);
			_deleteStream = true;

			if (!static_cast<std::ofstream*>(_out)->is_open())
				throw std::runtime_error("Failed to open log file.");
		}

		// destructor
		~Logger()
		{
			if (_toFile)
				flushToFile();
			
			if (_deleteStream)
				delete _out;
		}

		// function template for logging
		template <typename T>
		void log(const std::string& level, const T& message)
		{
			_buffer << "[" << getCurrentTime() << "] " 
				  << "[" << level << "] " 
				  << message << "\n";
		}

		void flushToFile()
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

		// overload << operator for streaming
		template <typename T>
		Logger& operator<<(const T& message)
		{
			_buffer << message;
			return *this;
		}

		// overload << operator for stream manipulators (like std::endl)
		Logger& operator<<(std::ostream& (*func)(std::ostream&))
		{
			if (func == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
				_buffer << "\n";
			return *this;
		}
};
