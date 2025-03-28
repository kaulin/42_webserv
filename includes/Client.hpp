#pragma once

#include <memory>
#include <ctime>
#include "Request.hpp"
#include "Response.hpp"
// #include "RequestHandler.hpp"
#include "ResponseHandler.hpp"

struct Client {
	int									fd;
	std::string							requestString;
	bool								requestReady;
	std::unique_ptr<HttpRequest>		request;
	std::unique_ptr<Response>			response;
	// std::unique_ptr<RequestHandler>		requestHandler;
	std::unique_ptr<ResponseHandler>	responseHandler;
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