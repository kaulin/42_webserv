#include "ServerException.hpp"

ServerException::ServerException(const eStatusCode& statusCode) : _statusCode(statusCode) {}

const char* ServerException::what() const noexcept {
	return statusMessage(_statusCode);
}

int ServerException::statusCode() const {
	return _statusCode;
}

const char* ServerException::statusMessage(int statusCode) {
	switch (statusCode) {
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 202:
			return "Accepted";
		case 204:
			return "No Content";
		case 301:
			return "Moved Permanentlly";
		case 307:
			return "Temporary Redirect";
		case 308:
			return "Permanent Redirect";
		case 400:
			return "Bad Request";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 408:
			return "Request Timeout";
		case 411:
			return "Length Required";
		case 413:
			return "Request Too Large";
		case 414:
			return "Request-URI Too Long";
		case 415:
			return "Unsupported Media Type";
		case 418:
			return "I'm a teapot";
		case 431:
			return "Request Header Fields Too Large";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Unsupported Method";
		case 503:
			return "Service Unavailable";
	}
	return "Internal Server Error";
}