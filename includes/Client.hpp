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
	int									fd;
	std::string							requestString;
	bool								requestReady;
	const Config*								serverConfig;
	std::unique_ptr<HttpRequest>		request;
	std::unique_ptr<HttpResponse>		response;
	std::unique_ptr<ResponseHandler>	responseHandler;
	std::unique_ptr<RequestHandler>		requestHandler;
	int									fileSize;
	int									fileReadFd;
	int									fileTotalBytesRead;
	int									fileWriteFd;
	int									fileTotalBytesWritten;
	std::string							responseBodyString;
	int									responseCode;
	bool								responseReady;
	std::time_t							lastRequest;
	bool								keep_alive;
};