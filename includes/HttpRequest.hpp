#pragma once

#include <iostream>
#include <unordered_map>

struct HttpRequest
{
	std::string method;										// "GET", "POST", "DELETE"
	std::string uri;										// e.g. "/index.html"
	std::string uriQuery;									// query string portion of the URI
	std::string	uriPath;									// only the path portion of the URI
	std::string httpVersion;								// e.g. "HTTP/1.1"
	std::unordered_map<std::string, std::string> headers;	// Header fields
	std::string body;										// Request body for POST (or PUT) requests
};