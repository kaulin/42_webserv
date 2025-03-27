#pragma once

struct Response
{
	int			statusCode;
	std::string	statusLine;
	std::string	body;
	std::string	responseString;
};
