#include <sys/socket.h>
#include "FileHandler.hpp"
#include "ResponseHandler.hpp"
#include "ServerException.hpp"

// Constructor
ResponseHandler::ResponseHandler(Client& client) : _client(client) {}

// Deconstructor
ResponseHandler::~ResponseHandler() {}

void ResponseHandler::sendResponse() {
	if (!_client.responseReady)
		return;
	size_t bytesSent;
	size_t leftToSend = _response->response.size();

	bytesSent= send(_client.fd, _response->response.c_str(), leftToSend, MSG_NOSIGNAL);
	if (bytesSent <= 0)
		throw SendError();
	_response->response.erase(0, bytesSent);
	if (bytesSent == leftToSend)
	{
		std::cout << "Client " << _client.fd << " response sent!\n" << "\n";
		_client.responseSent = true;
	}
	else
		std::cout << "Client [" << _client.fd << "] sent " << bytesSent << " bytes, continuing...\n";
}

/*
Adds status line to response
*/
void ResponseHandler::addStatus() {
	_response->response = _client.requestHandler->getHttpVersion() + " " + std::to_string(_client.responseCode) + " " + ServerException::statusMessage(_client.responseCode) + "\r\n";
}

/*
Adds a header key-value pair to response 
*/
void ResponseHandler::addHeader(const std::string& key, const std::string& value)
{
	_response->response += key + ": " + value + "\r\n";
}

/*
Adds body to response, preceded by Content-Length header and "\r\n" sequence
*/
void ResponseHandler::addBody(const std::string& bodyString)
{	
	addHeader("Content-Length", std::to_string(bodyString.length()));
	_response->response += "\r\n" + bodyString;
}

void ResponseHandler::formResponse()
{
	if (_client.responseReady)
		return;
	const HttpRequest& request = _client.requestHandler->getRequest();
	_response = std::make_unique<HttpResponse>();
	addStatus();
	addHeader("Date", getTimeStamp());
	if (_client.responseCode >= 300)
		formErrorPage();
	else if (_client.cgiRequested)
		formCGI();
	else if (request.method == "GET")
		formGET();
	else if (request.method == "POST")
		formPOST();
	else if (request.method == "DELETE")
		formDELETE();
	else
		throw ServerException(STATUS_METHOD_UNSUPPORTED);
	std::cout << "Finished forming response string!\n";
	_client.responseReady = true;
}

void ResponseHandler::formGET() {
	std::cout << "Forming response: GET\n";
	addHeader("Content-Type", FileHandler::getMIMEType(_client.requestHandler->getUriPath()));
	addBody(_client.resourceString);
}

void ResponseHandler::formPOST() {
	std::cout << "Forming response: POST\n";
	addHeader("Content-Type", "application/json");
	addHeader("Location", _client.requestHandler->getUri());
	addBody("{\n  \"status\": \"success\",\n  \"message\": \"Resouce successfully created\",\n  \"resource_id\": " + _client.requestHandler->getUri() + "\n}");
}

void ResponseHandler::formDELETE() {
	std::cout << "Forming response: DELETE\n";
	addHeader("Content-Type", "application/json");
	addBody("{\n  \"status\": \"success\",\n  \"message\": \"Resouce successfully deleted\",\n  \"resource_id\": " + _client.requestHandler->getUri() + "\n}");
}

void ResponseHandler::formCGI() {
	std::cout << "Forming response: CGI";
	_response->response = _client.resourceString;
}

void ResponseHandler::formDirectoryListing() {
	std::cout << "Forming response: DirectoryListing";
}

void ResponseHandler::formErrorPage() {
	std::cout << "Forming response: Error Page\n";
	addHeader("Content-Type", "text/html");
	if (_client.resourceString.empty()) {
		std::string code = std::to_string(_client.responseCode);
		std::string message = ServerException::statusMessage(_client.responseCode);

		_client.resourceString =  "<!DOCTYPE html>\n<html>\n<head>\n    <title>Error " + code + " - " + message + "</title>\n</head>\n<body>\n    <h1>" + code + "</h1>\n    <p>" + message + "</p>\n</body>\n</html>";
	}
	addBody(_client.resourceString);
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

const char* ResponseHandler::SendError::what() const noexcept {
	return "Send Failed";
}