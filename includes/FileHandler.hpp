#pragma once

#include <string>

class FileHandler {
	private:
		FileHandler();
		~FileHandler();
	public:
		static bool doesNotExist(const std::string& filePath);
		static bool isDirectory(const std::string& filePath);
		static void openForRead(int& fd, const std::string& filePath);
		static void openForWrite(int& fd, const std::string& filePath);
		static std::string getMIMEType(const std::string& filePath);
};