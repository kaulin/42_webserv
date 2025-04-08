#include <iostream>
#include <string>
#include <sys/socket.h>
#include "FileHandler.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"
#include "ServerException.hpp"
#include "ServerException.hpp"

RequestHandler::RequestHandler(Client& client) : 
	_client(client),
	_requestString(""),
	_readReady(false)
	// _chunkedRequest(false)
	// _chunkedRequestReady(false) 
	{}

RequestHandler::~RequestHandler() {}

void RequestHandler::resetHandler() {
	_requestString = "";
	_readReady = false;
}

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
	checkMethod();
	if (_request->uri.find(".py") != std::string::npos) // for testing CGI -- if request is to cgi-path
		_client.cgiRequested = true;
	else if (_request->method == "GET")
		processGet();
	else if (_request->method == "POST")
		processPost();
	else if (_request->method == "DELETE")
		processDelete();
	else
		throw ServerException(STATUS_METHOD_UNSUPPORTED);
}

void RequestHandler::processGet() {
	std::string path = "var/www/html" + _request->uri;
	FileHandler::openForRead(_client.fileReadFd, path);
	std::cout << "Requested file path: " << path << "\n";
}

void RequestHandler::processPost() {
	std::string path = "var/www/html" + _request->uri;
	FileHandler::openForWrite( _client.fileWriteFd, path);
	std::cout << "Requested file path: " << path << "\n";
}

void RequestHandler::processDelete() {
	throw ServerException(STATUS_METHOD_UNSUPPORTED);
}

void RequestHandler::checkMethod() const {
	const Config* config = _client.serverConfig;
	const std::string& method = getMethod();
	std::string path = getUriPath();
	std::cout << "Checking path [" << path << "] for method [" << method << "]\n";
	while(!path.empty())
	{
		path.erase(path.begin() + path.rfind('/'));
		auto it = config->_location.find(path);
		if (it != config->_location.end()) {
			std::cout << "	Found configuration for location [" << path << "], with method [" << method << "] set to: " << (*it).second._methods.at(method) <<  "\n";
			if ((*it).second._methods.at(method))
				return;
			throw ServerException(STATUS_NOT_ALLOWED);
		}
		std::cout << "	No configuration for [" << path << "], continuing search\n";
	}
	std::cout << "	No configuration for [" << path << "] found, method not allowed\n";
	throw ServerException(STATUS_NOT_ALLOWED);
}

// GETTERS
const HttpRequest& RequestHandler::getRequest() const { return *_request; }
const std::string& RequestHandler::getMethod() const { return _request->method; }
const std::string& RequestHandler::getUri() const { return _request->uri; }
const std::string& RequestHandler::getUriQuery() const { return _request->uriQuery; }
const std::string& RequestHandler::getUriPath() const { return _request->uriPath; }
const std::string& RequestHandler::getHttpVersion() const { return _request->httpVersion; }
const std::string& RequestHandler::getBody() const { return _request->body; }


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