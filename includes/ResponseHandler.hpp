// Response.hpp
#pragma once

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <string>
#include <ctime>
#include <memory>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Client.hpp"
#include "HttpServer.hpp"

struct Request;
struct Client;

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

class ResponseHandler
{
private:
	const Client& _client;
	const HttpRequest& _request;
	std::unique_ptr<HttpResponse> _response;
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
	ResponseHandler(const Client& client, const HttpRequest& request);
	~ResponseHandler(); 
	void formResponse();
	void sendResponse(int clientFd);
	
	// RESPONSE EXCEPTION CLASSES BELOW HERE
};

