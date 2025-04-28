#include <filesystem>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <algorithm>
#include "FileHandler.hpp"
#include "RequestHandler.hpp"
#include "RequestParser.hpp"
#include "ServerException.hpp"
#include "CGIHandler.hpp"

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
	_multipart = false;
	_partIndex = 0;
	_parts.clear();
	_request = nullptr;
}

void RequestHandler::handleRequest() {
	if (!_request)
		_request = std::make_unique<HttpRequest>();
	if (!_readReady)
		readRequest();
	else if (_multipart && ++_partIndex < _parts.size()) {
		_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath + "/" + _parts[_partIndex].filename;
		_client.resourceOutString = _parts[_partIndex].content;
		FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
	}
	else if (_client.cgiRequested && _client.cgiStatus != CGI_RESPONSE_READY) {
		return;
	}
	else
		_client.requestReady = true;
	
}

void RequestHandler::readRequest() {
	int receivedBytes;
	char buf[BUFFER_SIZE] = {};

	receivedBytes = recv(_client.fd, buf, BUFFER_SIZE, 0);
	if (receivedBytes == -1)
		throw ServerException(STATUS_RECV_ERROR);
	if (receivedBytes == 0)
		throw ServerException(STATUS_DISCONNECTED);
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

	auto it = _request->headers.find("Connection");
	if (it != _request->headers.end() && it->second == "close")
		_client.keep_alive = false;
	
	// TODO Check redirects
	std::cout << "uliuli:\n";
	for (auto& location : _client.serverConfig->locations)
	{
		std::cout << "location path: " << location.first << "\n";
		if (!location.second.redirect.second.empty()) {
			std::string redirect = location.second.redirect.second;
			int statusCode = location.second.redirect.first; // saving this for potential use
			(void)statusCode;
			std::cout << "redirect path:" << redirect << "\n";
			if (_request->uriPath == location.first) {
				// send redirection response with corresponding status code
				std::cout << "switcheroo\n";
				redirect.erase(redirect.begin(), redirect.begin() + redirect.find_last_of('/'));
				_request->uriPath = redirect; // this would be easy but probably wrong
			}
		}
	}
	std::cout << "uri path: " << _request->uriPath << "\n";

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
	// Check if request has Accept header and that requested resource matches said content type (TODO: add default "*/*")
	// auto itAcceptHeader = _request->headers.find("Accept");
	// if (itAcceptHeader != _request->headers.end() && (*itAcceptHeader).second.find(FileHandler::getMIMEType(_request->uriPath)) == std::string::npos)
	// 	throw ServerException(STATUS_NOT_ACCEPTABLE);
	_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, path) + path;
	FileHandler::openForRead(_client.resourceReadFd, _client.resourcePath);
}

void RequestHandler::processPost() {
	if (isMultipartForm())
		processMultipartForm();
	// if ((*itContentTypeHeader).second != FileHandler::getMIMEType(_request->uriPath))
	// 	throw ServerException(STATUS_BAD_REQUEST);
	else {
		_client.resourceOutString = _request->body;
		_client.resourcePath = ServerConfigData::getRoot(*_client.serverConfig, _request->uriPath) + _request->uriPath;
		FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
	}
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

bool RequestHandler::isMultipartForm() const {
	if (_request->headers.at("Content-Type").find("multipart/form-data") == 0)
		return true;
	return false;
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
	FileHandler::openForWrite( _client.resourceWriteFd, _client.resourcePath);
}

// GETTERS
const HttpRequest& RequestHandler::getRequest() const { return *_request; }
const std::string& RequestHandler::getMethod() const { return _request->method; }
const std::string& RequestHandler::getUri() const { return _request->uri; }
const std::string& RequestHandler::getUriQuery() const { return _request->uriQuery; }
const std::string& RequestHandler::getUriPath() const { return _request->uriPath; }
const std::string& RequestHandler::getHttpVersion() const { return _request->httpVersion; }
const std::string& RequestHandler::getBody() const { return _request->body; }
bool RequestHandler::getReadReady() const { return _readReady; }
const std::vector <MultipartFormData>& RequestHandler::getParts() const { return _parts; }

