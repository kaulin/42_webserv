#pragma once

#include <string>
#include <unordered_map>

// Struct to hold the request lines
struct HttpRequest
{
	std::string method;										// "GET", "POST", "DELETE"
	std::string uri;										// e.g. "/index.html"
	std::string httpVersion;								// e.g. "HTTP/1.1"
	std::unordered_map<std::string, std::string> headers;	// Header fields
	std::string body;									// Request body for POST (or PUT) requests
};

// The http request parser class
class HttpRequestParser
{
private:
	bool parseRequestLine(const std::string& request_line, HttpRequest& request);	// Function to parse the request line
	bool parseHeaders(const std::string& headers_part, HttpRequest& request);		// Function to parse headers (key: value)
	void parseBody(const std::string& body_part, HttpRequest& request);				// Function to parse the body

public:
	HttpRequestParser();											// Default constructor
	~HttpRequestParser();											// Default destructor

	// Parses raw HTTP request string into an HttpRequest object
	bool parseRequest(const std::string& raw_request, HttpRequest& request);
};
