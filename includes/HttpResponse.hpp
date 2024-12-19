// HttpResponse.hpp
#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <unordered_map>
#include "HttpRequest.hpp"

struct Request;

enum e_status_code
{
	STATUS_SUCCESS = 200,
	STATUS_CREATED = 201,
	STATUS_NO_CONTENT = 204,
	STATUS_BAD_REQUEST = 400,
	STATUS_FORBIDDEN = 403,
	STATUS_NOT_FOUND = 404,
	STATUS_NOT_ALLOWED = 405,
	STATUS_LENGTH_REQUIRED = 411,
	STATUS_TOO_LARGE = 413,
	STATUS_URI_TOO_LONG = 414,
	STATUS_INTERNAL_ERROR = 500,
	STATUS_NOT_IMPLEMENTED = 501
};

// Temp placeholders
#define START_LINE "HTTP/1.1 200 OK\n"
#define BODY "<!DOCTYPE html><html><head><title>index.html</title></head><body><h1>Hello World!</h1><body></html>"

class HttpResponse
{
private:
	HttpRequest& _request;
	int _statusCode;
	std::string _statusLine;
	std::deque<std::string> _headerKeys;
	std::unordered_map<std::string, std::string> _headers;
	std::string _body;
public:
	HttpResponse(HttpRequest& request);
	~HttpResponse(); 
	void setStatusCode(const int statusCode);
	void addHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	const std::string getStatusText() const;
	const std::string HttpResponse::toString() const;
	friend std::ostream& std::operator<<(std::ostream& os, const HttpResponse& obj);
};

/*
													Start line + headers = head
HTTP/1.1 200 OK									<-- Start line = <protocol> <status-code> <status-text (for humans)>
Date: Mon, 27 Jul 2009 12:28:53 GMT				<-- Headers start
Server: Apache/2.2.14 (Win32)
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
Content-Length: 88
Content-Type: text/html
Connection: Closed								<-- Headers end
												<-- Break line, signaling beginning of body
<!DOCTYPE html><html>...</html>					<-- Body

OR

HTTP/1.1 201 Created
Content-Type: application/json
Location: http://example.com/users/123

{
  "message": "New user created",
  "user": {
    "id": 123,
    "firstName": "Example",
    "lastName": "Person",
    "email": "bsmth@example.com"
  }
}

https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages

*/