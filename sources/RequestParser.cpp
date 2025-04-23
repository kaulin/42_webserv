#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include "RequestParser.hpp"
#include "HttpRequest.hpp"
#include "ServerException.hpp"

// Helper function to trim whitespace
std::string trimWhitespace(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");

	if (start == std::string::npos || end == std::string::npos)
		return "";

	return str.substr(start, end - start + 1);
}

// Function to parse the request line (e.g., "GET /index.html HTTP/1.1")
bool RequestParser::parseRequestLine(const std::string& request_line, HttpRequest& request)
{
	std::istringstream stream(request_line);

	// Extract method, URI, and version
	stream >> request.method >> request.uri >> request.httpVersion;

	// Validate the format: all three parts must be non-empty
	if (request.method.empty() || request.uri.empty() || request.httpVersion.empty())
		return false;  // Invalid request line

	for (size_t i = 0; i < request.method.length(); ++i)
		request.method[i] = std::toupper(request.method[i]);

	if (request.method != "GET" && request.method != "POST" && request.method != "DELETE")
		return false;  // Unsupported HTTP method

	request.uriPath = request.uri;
	size_t delimiter = request.uri.find("?");
	if (delimiter != std::string::npos)
	{
		request.uriPath = request.uri.substr(0, delimiter);
		request.uriQuery = request.uri.substr(delimiter + 1);
	}

	// Validate HTTP version (optional, for simplicity let's assume only HTTP/1.1 for now)
	if (request.httpVersion != "HTTP/1.1" && request.httpVersion != "HTTP/1.0")
		return false;  // Invalid HTTP version

	return true;
}


// Function to parse headers (key: value)
bool RequestParser::parseHeaders(const std::string& headers_part, HttpRequest& request)
{
	std::istringstream stream(headers_part);
	std::string line;

	while (std::getline(stream, line))
	{
		// Skip empty lines
		if (line.empty()) continue;

		// Find the position of the first colon, which separates the key and value
		size_t colon_pos = line.find(':');
		if (colon_pos == std::string::npos)
			return false;  // Invalid header format

		// Extract the header name and value, trimming spaces
		std::string key = line.substr(0, colon_pos);
		std::string value = line.substr(colon_pos + 1);

		// Trim leading and trailing spaces
		key = trimWhitespace(key);
		value = trimWhitespace(value);

		// Insert into the headers map
		request.headers[key] = value;
	}
	return true;
}

std::string RequestParser::parseChunkedBody(const std::string& chunked)
{
	std::istringstream stream(chunked);
	std::string decoded;

	while (true) {
		std::string sizeLine;
		if (!std::getline(stream, sizeLine))
			throw ServerException(STATUS_BAD_REQUEST); // incomplete chunk header

		sizeLine = trimWhitespace(sizeLine);
		if (sizeLine.empty())
			continue;

		size_t chunkSize = 0;
		std::istringstream sizeStream(sizeLine);
		sizeStream >> std::hex >> chunkSize;

		if (chunkSize == 0)
			break;

		std::string chunk(chunkSize, '\0');
		stream.read(&chunk[0], chunkSize);

		if (stream.gcount() != static_cast<std::streamsize>(chunkSize))
			throw ServerException(STATUS_BAD_REQUEST);

		// validate trailing CRLF
		char cr, lf;
		stream.get(cr);
		stream.get(lf);

		if (cr != '\r' || lf != '\n')
			throw ServerException(STATUS_BAD_REQUEST);

		decoded += chunk;
	}
	return decoded;
}



void RequestParser::parseBody(const std::string& body_part, HttpRequest& request)
{
	auto it = request.headers.find("Transfer-Encoding");
	if (it != request.headers.end()) {
		std::string encoding = trimWhitespace(it->second);
		std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::tolower);

		if (encoding == "chunked") {
			request.body = parseChunkedBody(body_part);
			return;
		}
	}

	request.body = body_part;
}


// Parses the entire HTTP request string into the HttpRequest object
bool RequestParser::parseRequest(const std::string& raw_request, HttpRequest& request)
{
	// Split the raw request into request line, headers, and body
	size_t pos = raw_request.find("\r\n");  // Find the first line break (request line)
	if (pos == std::string::npos) return false;

	// Parse the request line
	std::string request_line = raw_request.substr(0, pos);
	if (!parseRequestLine(request_line, request))
		return false;

	// Find the headers section (between request line and body)
	size_t headers_start = pos + 2;  // Skip the \r\n
	size_t headers_end = raw_request.find("\r\n\r\n", headers_start);
	if (headers_end == std::string::npos) return false;

	std::string headers_part = raw_request.substr(headers_start, headers_end - headers_start);
	if (!parseHeaders(headers_part, request))
		return false;

	// The body is the remaining part after headers
	size_t body_start = headers_end + 4;  // Skip the \r\n\r\n
	if (body_start < raw_request.size())
	{
		std::string body_part = raw_request.substr(body_start);
		parseBody(body_part, request);
	}
	return true;
}
