#include <sys/socket.h>
#include "ResponseHandler.hpp"
#include "ServerException.hpp"

// Constructor
ResponseHandler::ResponseHandler(Client& client) : _client(client) {}

// Deconstructor
ResponseHandler::~ResponseHandler() {}

void ResponseHandler::sendResponse() {
	if (!_client.responseReady)
		return;
	char buf[BUFFER_SIZE] = {};
	int bytesSent;
	size_t leftToSend = _response->responseString.size() - _response->totalBytesSent;
	size_t bytesToSend = leftToSend > BUFFER_SIZE ? BUFFER_SIZE : leftToSend;
	_response->body.copy(buf, BUFFER_SIZE, _response->totalBytesSent);

	try {
		bytesSent= send(_client.fd, buf, bytesToSend, MSG_NOSIGNAL);
		if (bytesSent <= 0)
			throw std::runtime_error("SEND ERROR EXCEPTION: send failed");
		if (bytesSent < BUFFER_SIZE)
		{
			if (static_cast<std::string::size_type>(_response->totalBytesSent) != _response->responseString.size())
				throw std::runtime_error("SEND ERROR EXCEPTION: send failed");
			std::cout << "Client " << _client.fd << " response sent:\n" << _response->responseString << "\n";
		}
		else
		{
			std::cout << "Client [" << _client.fd << "] sent " << bytesSent << " bytes socket, continuing...\n";
		}
	} catch (const std::runtime_error& e) {
		// log? or just std::cerr << e.what() << "\n";
		throw ServerException(STATUS_INTERNAL_ERROR);
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
	if (_client.responseReady)
		return;
	const HttpRequest& request = _client.requestHandler->getRequest();
	if (_response->statusCode >= 300)
		formErrorPage();
	else if (request.method == "GET")
		formGET();
	else if (request.method == "POST")
		formPOST();
	else if (request.method == "DELETE")
		formDELETE();
	else
		throw ServerException(STATUS_METHOD_UNSUPPORTED);
	_client.responseReady = true;
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
	const HttpRequest& request = _client.requestHandler->getRequest();
	std::cout << "Forming response: Error Page";
	std::string status = ServerException::statusMessage(_client.responseCode);
	_response->statusLine = request.httpVersion + " " + status + "\n";
	_response->body = "<html><head><title>" + status + "</title></head><body><center><h1>" + status + "</h1></center><hr><center>webserv</center></body></html>\n";
	addHeader("Server", "Webserv v0.6.6.6");
	addHeader("Content-Length", std::to_string(_response->body.size()));
	addHeader("Content-Type", "text/html");
	addHeader("Connection", "Closed");
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

const char* ResponseHandler::SendError::what() const noexcept {
	return "Send Failed";
}