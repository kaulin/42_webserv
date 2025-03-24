#include "Response.hpp"

// Constructor
Response::Response(HttpRequest& request) : 
	_request(request),
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

void Response::sendResponse(int clientFd) {
	std::string response = toString();
	int sendError;

	sendError = send(clientFd, response.c_str(), response.length(), MSG_NOSIGNAL);
	if (sendError <= 0)
		throw std::runtime_error("THROW SEND ERROR EXCEPTION");
	std::cout << "Client " << clientFd << " response:\n" << response << "\n";
}

/*
Adds a key-value pair to _header map and _headerKeys deque. If a header is 
re-inserted, it is only updated in the map.
*/
void Response::addHeader(const std::string& key, const std::string& value)
{
	if (_headers.find(key) == _headers.end())
		_headerKeys.emplace_front(key);
	_headers[key] = (value);
}

void Response::formResponse()
{
	if (_statusCode >= 300)
		formErrorPage();
	else if (_request.method == "GET")
		formGET();
	else if (_request.method == "POST")
		formPOST();
	else if (_request.method == "DELETE")
		formDELETE();
	else
		throw std::runtime_error("Method not implemented exception");
}

void Response::formGET() {
	std::cout << "Forming response: GET\n";
}

void Response::formPOST() {
	std::cout << "Forming response: POST\n";
}

void Response::formDELETE() {
	std::cout << "Forming response: DELETE\n";
}

void Response::formDirectoryListing() {
	std::cout << "Forming response: DirectoryListing";
}

void Response::formErrorPage() {
	std::cout << "Forming response: Error Page";
	std::string status = getStatus();
	_statusLine = _request.httpVersion + " " + status + "\n";
	_body = "<html><head><title>" + status + "</title></head><body><center><h1>" + status + "</h1></center><hr><center>webserv</center></body></html>\n";
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

/*
Static helper function for getting a timestamp in string format
*/
std::string Response::getTimeStamp()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
	return oss.str();
}

/*
Builds an HTTP response in the form of a single string
*/
const std::string Response::toString() const
{
	std::string response;
	
	response += _statusLine;
	response += "Date: " + getTimeStamp() + "\n";
	for (std::string key : _headerKeys)
		response += key + ": " + _headers.at(key) + "\n";
	response += "\n" + _body;
	return response;
}

// RESPONSE EXCEPTION IMPLEMENTATIONS BELOW HERE