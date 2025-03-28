#include "RequestHandler.hpp"
#include "RequestParser.hpp"
#include <iostream>

RequestHandler::RequestHandler(const std::string& raw_request)
{
	RequestParser::parseRequest(raw_request, _request);
}

const HttpRequest &RequestHandler::getRequest() const
{
    return _request;
}

const std::string &RequestHandler::getMethod() const { return _request.method; }
const std::string &RequestHandler::getUri() const { return _request.uri; }
const std::string &RequestHandler::getUriQuery() const { return _request.uriQuery; }
const std::string &RequestHandler::getUriPath() const { return _request.uriPath; }
const std::string &RequestHandler::getHttpVersion() const { return _request.httpVersion; }
const std::string &RequestHandler::getBody() const { return _request.body; }


// int main()
// {
//     std::string raw_request = "GET /index.html HTTP/1.1\r\n"
//                               "Host: example.com\r\n"
//                               "User-Agent: CustomClient/1.0\r\n"
//                               "\r\n";

//     RequestHandler req1(raw_request);
//     const HttpRequest &request = req1.getRequest();
//     std::cout << request.httpVersion << std::endl;
//     std::cout << request.uri << std::endl;
//     std::cout << request.method << std::endl;
//     std::cout << request.uriPath << std::endl;

//     std::cout << req1.getUri() << std::endl;
// }