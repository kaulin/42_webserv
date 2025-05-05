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

enum ConnectionState
{
	ACTIVE,
	DRAIN,
	DRAINED,
	CLOSE
};

struct Client {
	const Config*						serverConfig;
	int									fd;
	std::time_t							lastActivity;
	std::unique_ptr<ResponseHandler>	responseHandler;
	std::unique_ptr<RequestHandler>		requestHandler;
	std::string							resourcePath;
	std::string							resourceInString;
	std::string							resourceOutString;
	int									resourceReadFd;
	size_t								resourceBytesRead;
	int									resourceWriteFd;
	size_t								resourceBytesWritten;
	int									responseSent;
	int									responseCode;
	ConnectionState						connectionState;
	bool								keepAlive;
	bool								cgiRequested;
	bool								directoryListing;
	bool								requestReady;
	bool								responseReady;
	int									cgiStatus;
};