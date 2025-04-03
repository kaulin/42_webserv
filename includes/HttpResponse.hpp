#pragma once

#include <string>

#define BUFFER_SIZE 1024

struct HttpResponse
{
	int			totalBytesSent;
	std::string	response;
	bool		responseReady;
};
