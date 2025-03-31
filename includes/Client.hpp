#pragma once

#include <memory>
#include <ctime>
#include "RequestHandler.hpp"
#include "Response.hpp"
#include "ResponseHandler.hpp"

class ResponseHandler;

struct Client {
	int									fd;
	std::string							requestString;
	bool								requestReady;
	std::unique_ptr<HttpRequest>		request;
	std::unique_ptr<Response>			response;
	std::unique_ptr<ResponseHandler>	responseHandler;
	// std::unique_ptr<RequestHandler>		requestHandler;
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