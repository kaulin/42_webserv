// Response.hpp
#pragma once

#include <filesystem>
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

struct Request;
struct Client;

class ResponseHandler
{
private:
	Client& _client;
	size_t _totalBytesSent;
	std::unique_ptr<HttpResponse> _response;
	void formGET();
	void formPOST();
	void formDELETE();
	void formDirectoryListing();
	void formErrorPage();
	void formCGI();
	void addStatus();
	void addHeader(const std::string& key, const std::string& value);
	void addBody(const std::string& bodyString);
	static std::string timeToString(const std::filesystem::file_time_type& time);
	static std::string sizeToString(const size_t& size);
	static std::string getTimeStamp();
public:
	ResponseHandler(Client& client);
	~ResponseHandler();
	void resetHandler();
	void sendResponse();
	void formResponse();
};

