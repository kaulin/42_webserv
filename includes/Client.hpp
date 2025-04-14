#pragma once

#include <memory>
#include <ctime>
#include "HttpRequest.hpp"
#include "RequestHandler.hpp"
#include "HttpResponse.hpp"
#include "ResponseHandler.hpp"
#include "ServerConfigData.hpp"

struct Config;
class RequestHandler;
class ResponseHandler;

struct Client {
	const Config*						serverConfig;
	int									fd;
	bool								keep_alive;
	std::time_t							lastRequest;
	std::unique_ptr<ResponseHandler>	responseHandler;
	std::unique_ptr<RequestHandler>		requestHandler;
	std::string							resourcePath;
	std::string							resourceString;
	bool								cgiRequested;
	bool								directoryListing;
	bool								requestReady;
	bool								responseReady;
	int									responseSent;
	int									responseCode;
	int									fileReadFd;
	int									fileTotalBytesRead;
	int									fileWriteFd;
	int									fileTotalBytesWritten;
};