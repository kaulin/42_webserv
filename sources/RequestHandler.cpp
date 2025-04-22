#include <filesystem>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include "FileHandler.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"
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
	//throw ServerException(STATUS_INTERNAL_ERROR); // ServerException test
	if (receivedBytes <= 0)
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

	// TODO Check redirects

	std::cout << "Client " << _client.fd << " request method " << _request->method << " and URI: " << _request->uri << "\n";
	if (!ServerConfigData::checkMethod(*_client.serverConfig, _request->method, _request->uriPath))
		throw ServerException(STATUS_NOT_ALLOWED);
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
	std::string path = _request->uriPath;
	// Check if request is for a directory, check for default index, check for auto-index
	if (FileHandler::isDirectory(_client.serverConfig->root + _request->uriPath)) {
		const Location* location = ServerConfigData::getLocation(*_client.serverConfig, path);
		if (location != nullptr) {
			if (!location->index.empty())
				path = location->path + location->index;
			else if (location->dir_listing) {
				_client.directoryListing = true;
				_client.requestReady = true;
				_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, path) + path;
				return;
			}
			else
				throw ServerException(STATUS_FORBIDDEN);
		}
		else
			throw ServerException(STATUS_FORBIDDEN);
	}
	// Check if request has Accept header and that requested resource matches said content type (TODO: add default "*/*")
	// auto itAcceptHeader = _request->headers.find("Accept");
	// if (itAcceptHeader != _request->headers.end() && (*itAcceptHeader).second.find(FileHandler::getMIMEType(_request->uriPath)) == std::string::npos)
	// 	throw ServerException(STATUS_NOT_ACCEPTABLE);
	_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, path) + path;
	FileHandler::openForRead(_client.resourceReadFd, _client.resourcePath);
}

void RequestHandler::processPost() {
	// Check if requested resource file type matches with Content-Type header
	auto itContentTypeHeader = _request->headers.find("Content-Type");
	if (itContentTypeHeader == _request->headers.end() || (*itContentTypeHeader).second != FileHandler::getMIMEType(_request->uriPath))
		throw ServerException(STATUS_BAD_REQUEST);
	_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath;
	FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
}

void RequestHandler::processDelete() {
	std::string file = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath;
	if (!std::filesystem::exists(file))
		throw ServerException(STATUS_NOT_FOUND);
	if (!std::filesystem::is_regular_file(file))
		throw ServerException(STATUS_FORBIDDEN);
	_client.resourcePath = file;
	_client.requestReady = true;
}

// GETTERS
const HttpRequest& RequestHandler::getRequest() const { return *_request; }
const std::string& RequestHandler::getMethod() const { return _request->method; }
const std::string& RequestHandler::getUri() const { return _request->uri; }
const std::string& RequestHandler::getUriQuery() const { return _request->uriQuery; }
const std::string& RequestHandler::getUriPath() const { return _request->uriPath; }
const std::string& RequestHandler::getHttpVersion() const { return _request->httpVersion; }
const std::string& RequestHandler::getBody() const { return _request->body; }
