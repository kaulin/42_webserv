#include <sys/socket.h>
#include "Response.hpp"
#include "ResponseHandler.hpp"

// Constructor
ResponseHandler::ResponseHandler(const Client& client, const HttpRequest& request) : 
	_client(client),
	_request(request)
{
	// checkRequest(); // check request errors (eg POST with no type or transfer encoding)
	// if (!_resolved) checkMethod(); // check method & allowed methods at location
	// if (!_resolved) checkResource(); // check if resource exists
	// if (!_resolved) checkCGI; // check cgi, execute cgi
	// if (!_resolved) handleMethod; // handle method
	// if (!_resolved) _statusCode = STATUS_INTERNAL_ERROR;
}

// Deconstructor
ResponseHandler::~ResponseHandler() {}

void ResponseHandler::sendResponse(int clientFd) {
	char buf[BUFFER_SIZE] = {};
	int bytesSent;
	size_t leftToSend = _response->responseString.size() - _response->totalBytesSent;
	size_t bytesToSend = leftToSend > BUFFER_SIZE ? BUFFER_SIZE : leftToSend;
	_response->body.copy(buf, BUFFER_SIZE, _response->totalBytesSent);

	try {
		bytesSent= send(clientFd, buf, bytesToSend, MSG_NOSIGNAL);
		if (bytesSent <= 0)
			throw std::runtime_error("SEND ERROR EXCEPTION: send failed");
		if (bytesSent < BUFFER_SIZE)
		{
			if (_response->totalBytesSent != _response->responseString.size())
				throw std::runtime_error("SEND ERROR EXCEPTION: send failed");
			std::cout << "Client " << clientFd << " response sent:\n" << _response->responseString << "\n";
		}
		else
		{
			std::cout << "Client [" << clientFd << "] sent " << bytesSent << " bytes socket, continuing...\n";
		}
	} catch (const std::runtime_error& e) {
		// handle send error exception
	}
}

/*
Adds a key-value pair to _header map and _headerKeys deque. If a header is 
re-inserted, it is only updated in the map.
*/
void ResponseHandler::addHeader(const std::string& key, const std::string& value)
{
	_response->headers += key + ": " + value + "\n";
}

void ResponseHandler::formResponse()
{
	if (_response->statusCode >= 300)
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

void ResponseHandler::formGET() {
	std::cout << "Forming response: GET\n";
}

void ResponseHandler::formPOST() {
	std::cout << "Forming response: POST\n";
}

void ResponseHandler::formDELETE() {
	std::cout << "Forming response: DELETE\n";
}

void ResponseHandler::formDirectoryListing() {
	std::cout << "Forming response: DirectoryListing";
}

void ResponseHandler::formErrorPage() {
	std::cout << "Forming response: Error Page";
	std::string status = getStatus();
	_response->statusLine = _request.httpVersion + " " + status + "\n";
	_response->body = "<html><head><title>" + status + "</title></head><body><center><h1>" + status + "</h1></center><hr><center>webserv</center></body></html>\n";
	addHeader("Server", "Webserv v0.6.6.6");
	addHeader("Content-Length", std::to_string(_response->body.size()));
	addHeader("Content-Type", "text/html");
	addHeader("Connection", "Closed");
}

const std::string ResponseHandler::getStatus() const {
	switch (_response->statusCode)
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
std::string ResponseHandler::getTimeStamp()
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
const std::string ResponseHandler::toString() const
{
	std::string response;
	
	response += _response->statusLine;
	response += _response->headers;
	response += _response->body;
	return response;
}

// RESPONSE EXCEPTION IMPLEMENTATIONS BELOW HERE