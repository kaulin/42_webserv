#include <fcntl.h>
#include <filesystem>
#include <unordered_map>
#include "FileHandler.hpp"
#include "ServerException.hpp"

FileHandler::FileHandler() {}
FileHandler::~FileHandler() {}

bool FileHandler::doesNotExist(const std::string& filePath) {
	return !std::filesystem::exists(filePath);
}

bool FileHandler::isDirectory(const std::string& filePath) {
	return std::filesystem::is_directory(filePath);
}

void FileHandler::openForRead(int& fd, const std::string& filePath) {
	if (doesNotExist(filePath))
		throw ServerException(STATUS_NOT_FOUND);
	fd = open(filePath.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd < 0)
		throw ServerException(STATUS_FORBIDDEN);
}

void FileHandler::openForWrite(int& fd, const std::string& filePath) {
	fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
	if (fd < 0)
		throw ServerException(STATUS_FORBIDDEN);
}

std::string FileHandler::getMIMEType(const std::string& filePath) {
	size_t extensionStart = filePath.find_last_of(".");
	if (extensionStart == std::string::npos)
		throw ServerException(STATUS_TYPE_UNSUPPORTED);
	std::string extension = filePath.substr(extensionStart);
	
	// Defines MIME types, can be added to
	std::unordered_map<std::string, std::string> types = {
		{".html", "text/html"},
		{".htm", "text/html"},
		{".css", "text/css"},
		{".js", "application/javascript"},
		{".json", "application/json"},
		{".png", "image/png"},
		{".ico", "image/x-icon"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".gif", "image/gif"},
		{".bmp", "image/bmp"},
		{".svg", "image/svg+xml"},
		{".txt", "text/plain"},
		{".xml", "application/xml"},
		{".pdf", "application/pdf"},
		{".zip", "application/zip"},
		{".mp3", "audio/mpeg"},
		{".mp4", "video/mp4"},
		{".avi", "video/x-msvideo"},
		{".csv", "text/csv"},
		{".md", "text/markdown"}
	};

	auto type = types.find(extension);
	if (type == types.end())
		throw ServerException(STATUS_TYPE_UNSUPPORTED);
	return type->second;
}