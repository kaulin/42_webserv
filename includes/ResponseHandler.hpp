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

struct Request;
struct Client;

class ResponseHandler
{
private:
	Client& _client;
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
	void makeResponseString();
	static std::string getTimeStamp();
public:
	ResponseHandler(Client& client);
	~ResponseHandler();
	void sendResponse();
	void formResponse();
	
	class SendError : public std::exception {
		public:
			const char* what() const noexcept;
	};
};

