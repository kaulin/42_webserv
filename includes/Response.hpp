// Response.hpp
#pragma once

#include <string>

class Response
{
private:
	std::string _body;
	int _statusCode;
public:
	Response(/* args */);
	~Response();
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