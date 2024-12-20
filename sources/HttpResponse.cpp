#include "HttpResponse.hpp"

static std::string timeStamp()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
	return oss.str();
}

// Constructor
HttpResponse::HttpResponse(HttpRequest& request) : 
	_request(request),
	_resolved(false), 
	_statusCode(0)
{
	checkRequest(); // check request errors (eg POST with no type or transfer encoding)
	if (!_resolved) checkMethod(); // check method & allowed methods at location
	if (!_resolved) checkResource(); // check if resource exists
	if (!_resolved) checkCGI; // check cgi, execute cgi
	if (!_resolved) handleMethod; // handle method
	if (!_resolved) _statusCode = STATUS_INTERNAL_ERROR;
	formResponse();
}

// Deconstructor
HttpResponse::~HttpResponse() {}

// Adds a key-value pair to _header map
void HttpResponse::addHeader(const std::string& key, const std::string& value)
{
	_headerKeys.push_back(key);
	_headers.insert(key, value);
}

void HttpResponse::checkRequest()
{
	_statusCode = STATUS_NOT_IMPLEMENTED;
	_resolved = true;
}

void HttpResponse::checkMethod()
{

}

void HttpResponse::checkResource()
{
	
}

void HttpResponse::checkCGI()
{
	
}

void HttpResponse::handleMethod()
{
	
}

void HttpResponse::formResponse()
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
			"</h1></center><hr><center>nginx/1.27.3</center></body></html>";
	}
	addHeader("Server", "Webserv v0.6.6.6");
	addHeader("Content-Length", std::to_string(_body.size()));
	addHeader("Content-Type", "text/html");
	addHeader("Connection", "Closed");
}

const std::string HttpResponse::getStatus() const {
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
const std::string HttpResponse::toString() const
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

// Stream operator overload, for writing the 
std::ostream& std::operator<<(std::ostream& os, const HttpResponse& response)
{
	os << response.toString();
	return os;
}