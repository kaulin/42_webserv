#include <iostream>
#include <sys/socket.h>
#include "ServerException.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"

RequestHandler::~RequestHandler() {}

RequestHandler::RequestHandler(Client& client) : 
	_client(client),
	_requestString(""),
	_requestReady(false)
	// _chunkedRequest(false),
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
			_requestReady = true;
			std::cout << "Client [" << _client.fd << "] request ready: " << _requestString << "\n";
			processRequest();
		}
		else // more incoming
			std::cout << "Received request portion: " << buf << "\n";
	}
}

void RequestHandler::processRequest() {
	_request = std::make_unique<HttpRequest>();
	
	RequestParser::parseRequest(_requestString, *_request.get());
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
//     std::string raw_request = "GET /index.html HTTP/1.1\r\n"
//                               "Host: example.com\r\n"
//                               "User-Agent: CustomClient/1.0\r\n"
//                               "\r\n";

//     RequestHandler req1(raw_request);
//     const HttpRequest &request = req1.getRequest();
//     std::cout << request.httpVersion << std::endl;
//     std::cout << request.uri << std::endl;
//     std::cout << request.method << std::endl;
//     std::cout << request.uriPath << std::endl;

//     std::cout << req1.getUri() << std::endl;
// }