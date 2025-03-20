#include "Response.hpp"

static std::string timeStamp()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
	return oss.str();
}

// Constructor
Response::Response(HttpRequest& request) : 
	_request(request),
	_resolved(false), 
	_statusCode(0)
{
	// checkRequest(); // check request errors (eg POST with no type or transfer encoding)
	// if (!_resolved) checkMethod(); // check method & allowed methods at location
	// if (!_resolved) checkResource(); // check if resource exists
	// if (!_resolved) checkCGI; // check cgi, execute cgi
	// if (!_resolved) handleMethod; // handle method
	// if (!_resolved) _statusCode = STATUS_INTERNAL_ERROR;
	formResponse();
}

// Deconstructor
Response::~Response() {}

// Adds a key-value pair to _header map
void Response::addHeader(const std::string& key, const std::string& value)
{
	_headers.insert(key, value);
}

void Response::formResponse()
{
	std::string status = getStatus();

	_statusLine = _request.http_version + " " + status + "\n";
	if (_statusCode >= 300)
	{ 
		_body = 
			"<html><head><title>" +
			status +
			"</title></head><body><center><h1>" +
			status +
			"</h1></center><hr><center>webserv</center></body></html>";
	}
	addHeader("Server", "Webserv v0.6.6.6");
	addHeader("Content-Length", std::to_string(_body.size()));
	addHeader("Content-Type", "text/html");
	addHeader("Connection", "Closed");
}

const std::string Response::getStatus() const {
	switch (_statusCode)
	{
		case 200:
			return ("200 OK");
		case 201:
			return ("201 Created");
		case 204:
			return ("204 No Content");
		case 400:
			return ("400 Bad Request");
		case 403:
			return ("403 Forbidden");
		case 404:
			return ("404 Not Found");
		case 405:
			return ("405 Method Not Allowed");
		case 413:
			return ("413 Content Too Large");
		case 414:
			return ("414 URI Too Long");
		case 500:
			return ("500 Internal Server Error");
		default :
			return ("501 Not Implemented");

	}
}

// Builds an HTTP response in the form of a single string
const std::string Response::toString() const
{
	std::string response;
	
	response += _statusLine;
	response += "Date: " + timeStamp() + "\n";
	for (std::string key : _headerKeys)
	{
		response += key + ": " + _headers.at(key) + "\n";
	}
	response += "\n" + _body;
	return response;
}

// RESPONSE EXCEPTION IMPLEMENTATIONS BELOW HERE