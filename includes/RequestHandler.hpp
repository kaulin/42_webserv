#pragma once

#include <string>
#include <unordered_map>

struct HttpRequest
{
	std::string method;										// "GET", "POST", "DELETE"
	std::string uri;										// e.g. "/index.html"
	std::string uriQuery;									// query string portion of the URI
	std::string	uriPath;									// only the path portion of the URI
	std::string httpVersion;								// e.g. "HTTP/1.1"
	std::unordered_map<std::string, std::string> headers;	// Header fields
	std::string body;										// Request body for POST (or PUT) requests
};

class RequestHandler
{
	protected:
		HttpRequest _request;

	public:
		RequestHandler() = delete;
		RequestHandler(const std::string& raw_request);
		~RequestHandler() = default;								// Default destructor

		const HttpRequest &getRequest() const;						// const ref to whole struct
		const std::string &getMethod() const;
		const std::string &getUri() const;
		const std::string &getUriQuery() const;
		const std::string &getUriPath() const;
		const std::string &getHttpVersion() const;
		const std::string &getBody() const;

};
