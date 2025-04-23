#pragma once

#include <exception>

enum eStatusCode
{
	STATUS_OK = 200,
	STATUS_CREATED = 201,
	STATUS_ACCEPTED = 202,
	STATUS_NO_CONTENT = 204,
	STATUS_MOVED = 301,
	STATUS_TEMP_REDIR = 307,
	STATUS_PERM_REDIR = 308,
	STATUS_BAD_REQUEST = 400,
	STATUS_FORBIDDEN = 403,
	STATUS_NOT_FOUND = 404,
	STATUS_NOT_ALLOWED = 405,
	STATUS_NOT_ACCEPTABLE = 406,
	STATUS_TIMEOUT = 408,
	STATUS_LENGTH_REQUIRED = 411,
	STATUS_TOO_LARGE = 413,
	STATUS_URI_TOO_LONG = 414,
	STATUS_TYPE_UNSUPPORTED = 415,
	STATUS_TEAPOT = 418,
	STATUS_LARGE_HEADERS = 431,
	STATUS_DISCONNECTED = 499,
	STATUS_INTERNAL_ERROR = 500,
	STATUS_METHOD_UNSUPPORTED = 501,
	STATUS_RECV_ERROR = 665,
	STATUS_SEND_ERROR = 666
};

class ServerException : public std::exception
{
	private:
		eStatusCode _statusCode;
	public:
		ServerException(const eStatusCode& statusCode);
		const char* what() const noexcept;
		int statusCode() const;
		static const char* statusMessage(int statusCode);
};