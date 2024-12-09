// HttpResponse.hpp
#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <unordered_map>

// Temp placeholders
#define START_LINE "HTTP/1.1 200 OK\n"
#define BODY "<!DOCTYPE html><html><head><title>index.html</title></head><body><h1>Hello World!</h1><body></html>"

class HttpResponse
{
private:
	int _statusCode;
	std::deque<std::string> _headerKeys;
	std::unordered_map<std::string, std::string> _headers;
	std::string _body;
public:
	HttpResponse();
	~HttpResponse(); 
	void setStatusCode(const int statusCode);
	void addHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	const std::string HttpResponse::buildResponse() const;
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