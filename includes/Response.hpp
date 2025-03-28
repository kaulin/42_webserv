#pragma once

struct Response
{
	int			statusCode;
	int			totalBytesSent;
	std::string	statusLine;
	std::string	headers;
	std::string	body;
	std::string	responseString;
	bool		responseReady;
};
