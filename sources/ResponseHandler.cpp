#include <chrono>
#include <sys/socket.h>
#include "FileHandler.hpp"
#include "ResponseHandler.hpp"
#include "ServerException.hpp"

// Constructor
ResponseHandler::ResponseHandler(Client& client) : _client(client) {}

// Deconstructor
ResponseHandler::~ResponseHandler() {}

void ResponseHandler::sendResponse() {
	if (!_client.responseReady)
		return;
	size_t bytesSent;
	size_t leftToSend = _response->response.size();

	bytesSent= send(_client.fd, _response->response.c_str(), leftToSend, MSG_NOSIGNAL);
	if (bytesSent <= 0)
		throw SendError();
	_response->response.erase(0, bytesSent);
	if (bytesSent == leftToSend)
	{
		std::cout << "Client " << _client.fd << " response sent!\n" << "\n";
		_client.responseSent = true;
	}
	else
		std::cout << "Client [" << _client.fd << "] sent " << bytesSent << " bytes, continuing...\n";
}

/*
Adds status line to response
*/
void ResponseHandler::addStatus() {
	_response->response = _client.requestHandler->getHttpVersion() + " " + std::to_string(_client.responseCode) + " " + ServerException::statusMessage(_client.responseCode) + "\r\n";
}

/*
Adds a header key-value pair to response 
*/
void ResponseHandler::addHeader(const std::string& key, const std::string& value)
{
	_response->response += key + ": " + value + "\r\n";
}

/*
Adds body to response, preceded by Content-Length header and "\r\n" sequence
*/
void ResponseHandler::addBody(const std::string& bodyString)
{	
	addHeader("Content-Length", std::to_string(bodyString.length()));
	_response->response += "\r\n" + bodyString;
}

void ResponseHandler::formResponse()
{
	if (_client.responseReady)
		return;
	const HttpRequest& request = _client.requestHandler->getRequest();
	_response = std::make_unique<HttpResponse>();
	if (_client.responseCode >= 300)
		formErrorPage();
	else if (_client.cgiRequested)
		formCGI();
	else if (request.method == "GET" && _client.directoryListing)
		formDirectoryListing();
	else if (request.method == "GET")
		formGET();
	else if (request.method == "POST")
		formPOST();
	else if (request.method == "DELETE")
		formDELETE();
	_client.responseReady = true;
}

void ResponseHandler::formGET() {
	addStatus();
	addHeader("Date", getTimeStamp());
	addHeader("Content-Type", FileHandler::getMIMEType(_client.resourcePath));
	addBody(_client.resourceString);
}

void ResponseHandler::formPOST() {
	addStatus();
	addHeader("Date", getTimeStamp());
	addHeader("Content-Type", "application/json");
	addHeader("Location", _client.requestHandler->getUri());
	addBody("{\n  \"status\": \"success\",\n  \"message\": \"Resouce successfully created\",\n  \"resource_id\": " + _client.requestHandler->getUri() + "\n}");
}

void ResponseHandler::formDELETE() {
	std::filesystem::path file(_client.resourcePath);
	std::error_code ec;
	if (std::filesystem::exists(file)) {
		if (!remove(file, ec))
			throw ServerException(STATUS_INTERNAL_ERROR);
	}
	if (ec.value() != 0 && std::filesystem::exists(file)) // can log ec if needed
		throw ServerException(STATUS_INTERNAL_ERROR);
	_client.responseCode = 204;
	addStatus();
	addHeader("Date", getTimeStamp());
	addBody("");
}

void ResponseHandler::formCGI() {
	_response->response = _client.resourceString;
}

void ResponseHandler::formDirectoryListing() {
	addStatus();
	addHeader("Date", getTimeStamp());
	addHeader("Content-Type", "text/html");
	size_t rootLen = ServerConfigData::getRoot(*_client.serverConfig, _client.resourcePath).size();
	std::ostringstream dirlistStream;
	std::ostringstream regularFileStream;
	std::string entryName;
	std::string entrySize;
	std::string entryLastWritten;
	dirlistStream << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n</head>\n";
	dirlistStream << "<body>\n<h1>Index of " << _client.requestHandler->getUriPath() <<  "</h1>\n<hr>\n";
	dirlistStream << "\n<table\n<tr>\n<th>Name</th>\n<th>Last Modified</th>\n<th>Size</th>\n</tr>\n";

	// Add link to parent directory
	std::string rootlessPath = _client.resourcePath.substr(rootLen);
	if (rootlessPath != "/") {
		if (rootlessPath.back() == '/')
			rootlessPath.erase(rootlessPath.end() - 1);
		rootlessPath.erase(rootlessPath.find_last_of('/') + 1);
		dirlistStream << "<tr>\n<td><a href=\"" << rootlessPath << "\" />../</a></td>\n<td></td>\n<td>-</td>\n</tr>\n";
	}

	// Add links to each file and subdirectory in current directory
	for (const std::filesystem::__cxx11::directory_entry& entry : std::filesystem::directory_iterator(_client.resourcePath))
	{
		entryName = entry.path().string().substr(entry.path().string().find_last_of("/") + 1);
		entrySize = entry.is_regular_file() ? sizeToString(entry.file_size()) : "-";
		entryLastWritten = timeToString(entry.last_write_time());
		rootlessPath = entry.path().string().substr(rootLen);
		
		if (entry.is_directory()) {
			dirlistStream << "<tr>\n<td><a href=\"" << rootlessPath << "\" />" << entryName << "/</a></td>\n";
			dirlistStream << "<td>" << entryLastWritten << "</td>\n";
			dirlistStream << "<td>" << entrySize << "</td>\n</tr>\n";
		}
		else {
			regularFileStream << "<tr>\n<td><a href=\"" << rootlessPath << "\" >" << entryName << "</a></td>\n";
			regularFileStream << "<td>" << entryLastWritten << "</td>\n";
			regularFileStream << "<td>" << entrySize << "</td>\n</td>\n</tr>\n";
		}
	}
	dirlistStream << regularFileStream.str() << "</table>\n</body>\n</html>\n";
	addBody(dirlistStream.str());
}

void ResponseHandler::formErrorPage() {
	addStatus();
	addHeader("Date", getTimeStamp());
	addHeader("Content-Type", "text/html");
	if (_client.resourceString.empty()) {
		std::string code = std::to_string(_client.responseCode);
		std::string message = ServerException::statusMessage(_client.responseCode);

		_client.resourceString =  "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n	<title>Error " + code + " - " + message + "</title>\n</head>\n<body>\n	<h1>" + code + "</h1>\n	<p>" + message + "</p>\n</body>\n</html>\n";
	}
	addBody(_client.resourceString);
}

std::string ResponseHandler::timeToString(const std::filesystem::file_time_type& time) {
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);
	std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
	std::ostringstream timeStream;
	timeStream << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M");
	return timeStream.str();
}

std::string ResponseHandler::sizeToString(const size_t& size) {
	const char* units[] = {"B", "KB", "MB", "GB"};
	int i = 0;
	double displaySize = static_cast<double>(size);
	while (displaySize > 1024 && i < 3) {
		displaySize /= 1024;
		++i;
	}
	std::ostringstream sizeStream;
	sizeStream << std::fixed << std::setprecision(1) << displaySize << " " << units[i];
	return sizeStream.str();
}

/*
Static helper function for getting a timestamp in string format
*/
std::string ResponseHandler::getTimeStamp()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream timeStream;
	timeStream << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
	return timeStream.str();
}

const char* ResponseHandler::SendError::what() const noexcept {
	return "Send Failed";
}