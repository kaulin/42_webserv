#include "HttpResponse.hpp"

// Constructor
HttpResponse::HttpResponse()
{
}


// Deconstructor
HttpResponse::~HttpResponse()
{
}


// Setter for int _statusCode
void HttpResponse::setStatusCode(const int statusCode) { _statusCode = statusCode; }

// Adds a key-value pair to _header map
void HttpResponse::addHeader(const std::string& key, const std::string& value)
{
	_headerKeys.push_back(key);
	_headers.insert(key, value);
}

// Setter of std::string _body
void HttpResponse::setBody(const std::string& body) { _body = body; }

// Builds an HTTP response in the form of a single string
const std::string HttpResponse::buildResponse() const
{
	std::string response;
	response += START_LINE;
	for (std::string key : _headerKeys)
	{
		response += key + ": " + _headers.at(key) + "\n";
	}
	response += "\n" + _body;
}

// Stream operator overload, for writing the 
std::ostream& std::operator<<(std::ostream& os, const HttpResponse& response)
{
	os << response.buildResponse();
	return os;
}