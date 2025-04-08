#include <filesystem>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include "ServerException.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"

RequestHandler::~RequestHandler() {}

RequestHandler::RequestHandler(Client& client) : 
	_client(client),
	_requestString(""),
	_readReady(false)
	// _chunkedRequest(false)
	// _chunkedRequestReady(false) 
	{}

void RequestHandler::readRequest() {
	int receivedBytes;
	char buf[BUFFER_SIZE] = {};

	receivedBytes = recv(_client.fd, buf, BUFFER_SIZE, 0);
	if (receivedBytes < 0)
		throw ServerException(STATUS_INTERNAL_ERROR);
	// else if (receivedBytes == 0) // client disconnected? send no response and clean client data in server handler
	//	throw ServerException(STATUS_CLIENT_DISCONNECTED); // <- not yet implemented
	else {
		_requestString.append(buf, receivedBytes);
		if (receivedBytes < BUFFER_SIZE) { // whole request read
			_readReady = true;
			std::cout << "Client " << _client.fd << " complete request received!\n";
			processRequest();
		}
		else // more incoming
			std::cout << "Client " << _client.fd << " received request portion:\n" << buf << "\n";
	}
}

void RequestHandler::processRequest() {
	_request = std::make_unique<HttpRequest>();
	
	RequestParser::parseRequest(_requestString, *_request.get());

	std::cout << "Client " << _client.fd << " request method " << _request->method << " and URI: " << _request->uri << "\n";

	if (_request->uri.find(".py") != std::string::npos) // for testing CGI -- if request is to cgi-path
	{
		_client.cgiRequested = true;
	}
	else if (_request->method == "GET")
	{
		std::string path = "var/www/html" + _request->uri;
		if (!std::filesystem::exists(path))
			throw ServerException(STATUS_NOT_FOUND);
		_client.fileReadFd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
		if (_client.fileReadFd < 0)
			throw ServerException(STATUS_FORBIDDEN);
		std::cout << "Requested file path: " << path << "\n";
	}

	if (_request->method == "POST")
	{
		std::string path = "var/www/html" + _request->uri;
		_client.fileWriteFd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
		std::cout << "Requested file path: " << path << "\n";
	}
}

void RequestHandler::resetHandler() {
	_requestString = "";
	_readReady = false;
}

std::string RequestHandler::getMIMEType(const std::string& filePath) {
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

const HttpRequest &RequestHandler::getRequest() const
{
	return *_request;
}

// Not sure if the getters below are needed, as most of the work will be done with the whole struct from above
const std::string &RequestHandler::getMethod() const { return _request->method; }
const std::string &RequestHandler::getUri() const { return _request->uri; }
const std::string &RequestHandler::getUriQuery() const { return _request->uriQuery; }
const std::string &RequestHandler::getUriPath() const { return _request->uriPath; }
const std::string &RequestHandler::getHttpVersion() const { return _request->httpVersion; }
const std::string &RequestHandler::getBody() const { return _request->body; }


// int main()
// {
//	 std::string raw_request = "GET /index.html HTTP/1.1\r\n"
//							   "Host: example.com\r\n"
//							   "User-Agent: CustomClient/1.0\r\n"
//							   "\r\n";

//	 RequestHandler req1(raw_request);
//	 const HttpRequest &request = req1.getRequest();
//	 std::cout << request.httpVersion << std::endl;
//	 std::cout << request.uri << std::endl;
//	 std::cout << request.method << std::endl;
//	 std::cout << request.uriPath << std::endl;

//	 std::cout << req1.getUri() << std::endl;
// }