#include <filesystem>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <algorithm>
#include "CGIHandler.hpp"
#include "FileHandler.hpp"
#include "Logger.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"
#include "ServerException.hpp"

RequestHandler::RequestHandler(Client& client) : _client(client) { resetHandler(); }

RequestHandler::~RequestHandler() {}

void RequestHandler::resetHandler() {
	_requestString = "";
	_headersRead = false;
	_readReady = false;
	_isChunked = false;
	_chunkedBodyStarted = false;
	_chunkState = SIZE;
	_expectedChunkSize = 0;
	_expectedContentLength = 0;
	_totalReceivedLength = 0;
	_multipart = false;
	_partIndex = 0;
	_parts.clear();
	_request = nullptr;
}

void RequestHandler::handleRequest() {
	if (!_request)
		_request = std::make_unique<HttpRequest>();
	readRequest();
	if (_multipart && ++_partIndex < _parts.size()) {
		_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath + "/" + _parts[_partIndex].filename;
		_client.resourceOutString = _parts[_partIndex].content;
		FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
	}
	else if (_multipart || (_client.cgiRequested && _client.cgiStatus == CGI_RESPONSE_READY))
		_client.requestReady = true;
}

void RequestHandler::readRequest() {
	ssize_t receivedBytes;
	char buf[BUFFER_SIZE] = {};

	receivedBytes = recv(_client.fd, buf, BUFFER_SIZE, 0);
	if (receivedBytes == -1)
	{
		Logger::log(Logger::ERROR, "Client " + std::to_string(_client.fd) + " recv error ");
		throw ServerException(STATUS_RECV_ERROR);
	}
	if (receivedBytes == 0)
		throw ServerException(STATUS_DISCONNECTED);
	if (_totalReceivedLength == 0)
		Logger::log(Logger::OK, "Client " + std::to_string(_client.fd) + " request incoming");
	_client.lastActivity = std::time(nullptr);
	_totalReceivedLength += receivedBytes;
	if (_client.connectionState == DRAIN)
	{
		if ((!_headersRead && receivedBytes < BUFFER_SIZE) || (_headersRead && _totalReceivedLength - _headerPart.length() >= _expectedContentLength))
		{
			_client.connectionState = DRAINED;
			_readReady = true;
		}
		return;
	}
	_requestString.append(buf, receivedBytes);
	handleHeaders();
	if (!_headersRead)
		return;
	if (_requestString.size() - _headerPart.size() > _client.serverConfig->cli_max_bodysize)
		throw ServerException(STATUS_TOO_LARGE);	
	if (_isChunked)
		handleChunkedRequest();
	else {
		if (_request->method == "POST" && _requestString.size() - _headerPart.size() > _expectedContentLength)
			throw ServerException(STATUS_BAD_REQUEST);
		if (_request->method != "POST" && _requestString.size() > _headerPart.size())
			throw ServerException(STATUS_BAD_REQUEST);
		if (receivedBytes < BUFFER_SIZE)
			_readReady = true;
	}
	if (_readReady)
		processRequest();
}

void RequestHandler::setContentLength() {
	auto it = _request->headers.find("Content-Length");
	if (it == _request->headers.end())
		throw ServerException(STATUS_LENGTH_REQUIRED);
	std::stringstream ss(it->second);
	ss >> _expectedContentLength;
	if (ss.fail() || !ss.eof())
		throw ServerException(STATUS_BAD_REQUEST);
	if (_expectedContentLength > _client.serverConfig->cli_max_bodysize)
		throw ServerException(STATUS_TOO_LARGE);
}

void RequestHandler::handleHeaders()
{
	if (_headersRead)
		return;
	size_t headersEnd = _requestString.find("\r\n\r\n");
	if (headersEnd > _client.serverConfig->cli_max_bodysize)
		throw ServerException(STATUS_LARGE_HEADERS);
	if (headersEnd == std::string::npos)
		return;

	_headersRead = true;
	_headerPart = _requestString.substr(0, headersEnd + 4);
	if (!RequestParser::parseRequest(_headerPart, *_request))
		throw ServerException(STATUS_BAD_REQUEST);

	std::string headerLower = _headerPart;
	std::transform(headerLower.begin(), headerLower.end(), headerLower.begin(), ::tolower);
	if (headerLower.find("transfer-encoding: chunked") != std::string::npos)
		_isChunked = true;
	if (!_isChunked && _request->method == "POST")
		setContentLength();
}

void RequestHandler::handleChunkedRequest()
{
	size_t headersEnd = _requestString.find("\r\n\r\n");
	if (!_chunkedBodyStarted) {
		if (headersEnd == std::string::npos)
			return;

		_chunkedBodyStarted = true;
		_chunkBuffer = _requestString.substr(headersEnd + 4);
		_requestString.clear();
	}

	while (true) {
		if (_chunkState == SIZE) {
			size_t pos = _chunkBuffer.find("\r\n");
			if (pos == std::string::npos)
				return; // need more data

			std::string sizeLine = _chunkBuffer.substr(0, pos);
			_chunkBuffer.erase(0, pos + 2);

			std::istringstream sizeStream(sizeLine);
			sizeStream >> std::hex >> _expectedChunkSize;

			if (_expectedChunkSize == 0) {
				_readReady = true;
				return;
			}
			_chunkState = DATA;
		}

		if (_chunkState == DATA) {
			if (_chunkBuffer.size() < _expectedChunkSize)
				return;

			_decodedBody += _chunkBuffer.substr(0, _expectedChunkSize);
			_chunkBuffer.erase(0, _expectedChunkSize);

			_chunkState = CRLF;
		}

		if (_chunkState == CRLF) {
			if (_chunkBuffer.size() < 2)
				return; // wait for trailing line break
			if (_chunkBuffer.substr(0, 2) != "\r\n")
				throw ServerException(STATUS_BAD_REQUEST);

			_chunkBuffer.erase(0, 2);
			_chunkState = SIZE;
		}
	}
}

void RequestHandler::processRequest() {
	if (_isChunked && !RequestParser::parseRequest(_headerPart + _decodedBody, *_request.get()))
			throw ServerException(STATUS_BAD_REQUEST);
	if (!_isChunked) {
		if (!RequestParser::parseRequest(_requestString, *_request.get()))
			throw ServerException(STATUS_BAD_REQUEST);
		if (_request->method == "POST" && _requestString.length() - _headerPart.length() != _expectedContentLength)
			throw ServerException(STATUS_BAD_REQUEST);
	}

	Logger::log(Logger::OK, "Client " + std::to_string(_client.fd) + " request received: " + _request->method + " " + _request->uri);
	_client.connectionState = PROCESSING;

	auto it = _request->headers.find("Connection");
	if (it != _request->headers.end() && it->second == "close")
		_client.keepAlive = false;
	
	if (checkRedirect())
		return;

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

bool RequestHandler::checkRedirect() {
	for (auto& location : _client.serverConfig->locations)
	{
		if (!location.second.redirect.second.empty()) {
			int statusCode = location.second.redirect.first;
			if (_request->uriPath == location.first) {
				_client.responseCode = statusCode;
				_client.requestReady = true;
				return true;
			}
		}
	}
	return false;
}

void RequestHandler::processGet() {
	std::string path = _request->uriPath;
	// Check if request is for a directory, check for default index, check for auto-index
	if (FileHandler::isDirectory(_client.serverConfig->root + _request->uriPath)) {
		const Location* location = ServerConfigData::getLocation(*_client.serverConfig, path);
		if (location == nullptr)
			throw ServerException(STATUS_FORBIDDEN);
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
	_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, path) + path;
	checkAcceptType();
	FileHandler::openForRead(_client.resourceReadFd, _client.resourcePath);
}

void RequestHandler::processPost() {
	checkContentLength();
	auto it = _request->headers.find("Content-Type");
	if (it == _request->headers.end())
		throw ServerException(STATUS_BAD_REQUEST);
	if (it->second.find("multipart/form-data") == 0)
		processMultipartForm();
	else {
		_client.resourceOutString = _request->body;
		_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath;
	}
	checkContentType();
	FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
}

void RequestHandler::processMultipartForm() {
	_multipart = true;
	std::string type = _request->headers.at("Content-Type");
	std::string prefix = "multipart/form-data; boundary=";
	if (type.find(prefix) == std::string::npos)
		throw ServerException(STATUS_BAD_REQUEST);
	std::string boundary = type.substr(prefix.size());
	RequestParser::parseMultipart(boundary, _request.get()->body, _parts);
	if (_parts.empty())
		throw ServerException(STATUS_BAD_REQUEST);
	_client.resourceOutString = _parts[_partIndex].content;
	_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath + "/" + _parts[_partIndex].filename;
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

void RequestHandler::checkContentType() const {
	if (_multipart)
	{
		for (MultipartFormData part : _parts)
		{
			if (FileHandler::getMIMEType(part.filename) != part.contentType)
				throw ServerException(STATUS_BAD_REQUEST);
		}
	}
	else if (FileHandler::getMIMEType(_client.resourcePath) != _request->headers.at("Content-Type"))
		throw ServerException(STATUS_BAD_REQUEST);
}

void RequestHandler::checkAcceptType() const {
	auto it = _request->headers.find("Accept");
	if (it == _request->headers.end() || it->second.find("*/*") != std::string::npos)
		return;
	std::string requestedResourceType = FileHandler::getMIMEType(_client.resourcePath);
	std::string requestedType = requestedResourceType.substr(0, requestedResourceType.find("/"));
	if (it->second.find(requestedResourceType) == std::string::npos && it->second.find(requestedType + "/*") == std::string::npos)
		throw ServerException(STATUS_NOT_ACCEPTABLE);
}

void RequestHandler::checkContentLength() const {
	auto it = _request->headers.find("Content-Length");
	if (it != _request->headers.end() && _expectedContentLength != _request->body.length())
		throw ServerException(STATUS_BAD_REQUEST);
}

// GETTERS
const HttpRequest& RequestHandler::getRequest() const { return *_request; }
const std::string& RequestHandler::getMethod() const { return _request->method; }
const std::string& RequestHandler::getUri() const { return _request->uri; }
const std::string& RequestHandler::getUriQuery() const { return _request->uriQuery; }
const std::string& RequestHandler::getUriPath() const { return _request->uriPath; }
const std::string& RequestHandler::getHttpVersion() const {
	if (_request->httpVersion.empty())
		_request->httpVersion = "HTTP/1.0";
	return _request->httpVersion;
}
const std::string& RequestHandler::getBody() const { return _request->body; }
bool RequestHandler::getReadReady() const { return _readReady; }
const std::vector <MultipartFormData>& RequestHandler::getParts() const { return _parts; }

