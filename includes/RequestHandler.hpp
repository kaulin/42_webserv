#pragma once

#include <string>
#include "HttpRequest.hpp"

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
