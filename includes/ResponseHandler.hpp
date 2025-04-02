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
	void sendResponse();
	
	class SendError : public std::exception {
		public:
			const char* what() const noexcept;
	};
};

