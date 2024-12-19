#include "HttpResponse.hpp"

// Constructor
HttpResponse::HttpResponse(HttpRequest& request) : _request(request), _statusCode(0)
{
	checkRequest(); // check request errors (eg POST with no type or transfer encoding)
	checkMethod(); // check method & allowed methods at location
	checkResource(); // check if resource exists
	checkCGI; // check cgi, execute cgi
	handleMethod; // handle method
}


// Deconstructor
HttpResponse::~HttpResponse() {}

// Adds a key-value pair to _header map
void HttpResponse::addHeader(const std::string& key, const std::string& value)
{
	_headerKeys.push_back(key);
	_headers.insert(key, value);
}

// Setter of std::string _body
void HttpResponse::setBody(const std::string& body) { _body = body; }

const std::string HttpResponse::getStatusText() const {
	switch (_statusCode)
	{
		case 200:
			return ("OK");
		case 201:
			return ("Created");
		case 204:
			return ("No Content");
		case 400:
			return ("Bad Request");
		case 403:
			return ("Forbidden");
		case 404:
			return ("Not Found");
		case 405:
			return ("Method Not Allowed");
		case 413:
			return ("Content Too Large");
		case 414:
			return ("URI Too Long");
		case 500:
			return ("Internal Server Error");
		default :
			return ("Not Implemented");

	}
}

// Builds an HTTP response in the form of a single string
const std::string HttpResponse::toString() const
{
	std::string response;
	
	response += START_LINE;
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