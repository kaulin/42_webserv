// Response.hpp
#pragma once

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <string>
#include <deque>
#include <unordered_map>
#include <ctime>
#include "Request.hpp"
#include "HttpServer.hpp"

struct Request;

enum e_status_code
{
	STATUS_SUCCESS = 200,
	STATUS_CREATED = 201,
	STATUS_NO_CONTENT = 204,
	STATUS_BAD_REQUEST = 400,
	STATUS_FORBIDDEN = 403,
	STATUS_NOT_FOUND = 404,
	STATUS_NOT_ALLOWED = 405,
	STATUS_LENGTH_REQUIRED = 411,
	STATUS_TOO_LARGE = 413,
	STATUS_URI_TOO_LONG = 414,
	STATUS_INTERNAL_ERROR = 500,
	STATUS_NOT_IMPLEMENTED = 501
};

// Temp placeholders
#define START_LINE "HTTP/1.1 200 OK\n"
#define BODY "<!DOCTYPE html><html><head><title>index.html</title></head><body><h1>Hello World!</h1><body></html>"

class Response
{
private:
	HttpRequest& _request;
	int _statusCode;
	std::string _statusLine;
	std::string _body;
	std::deque<std::string> _headerKeys;
	std::unordered_map<std::string, std::string> _headers;
	static std::string getTimeStamp();
	void formGET();
	void formPOST();
	void formDELETE();
	void formDirectoryListing();
	void formErrorPage();
	void addHeader(const std::string& key, const std::string& value);
	const std::string getStatus() const;
	const std::string toString() const;
public:
	Response(HttpRequest& request);
	~Response(); 
	void formResponse();
	void sendResponse(int clientFd);
	
	// RESPONSE EXCEPTION CLASSES BELOW HERE


};

