#pragma once

#include <memory>
#include <ctime>
#include "Request.hpp"
#include "Response.hpp"

struct Client {
	int								fd;
	std::string						requestString;
	bool							requestReady;
	std::unique_ptr<HttpRequest>	request;
	std::unique_ptr<Response>		response;
	int								fileSize;
	int								fileReadFd;
	int								fileTotalBytesRead;
	int								fileWriteFd;
	int								fileTotalBytesWritten;
	std::string						responseBodyString;
	int								responseCode;
	bool							responseReady;
	std::time_t						lastRequest;
	bool							keep_alive;
};