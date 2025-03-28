#pragma once

#include <string>
#include <unordered_map>
#include "RequestHandler.hpp"


// The http request parser class
class RequestParser : public RequestHandler
{
	private:
		// Struct to hold the request lines
		static bool parseRequestLine(const std::string& request_line, HttpRequest& request);	// Function to parse the request line
		static bool parseHeaders(const std::string& headers_part, HttpRequest& request);		// Function to parse headers (key: value)
		static void parseBody(const std::string& body_part, HttpRequest& request);				// Function to parse the body

	public:
		RequestParser() = default;											// Default constructor
		~RequestParser() = default;											// Default destructor

		// Parses raw HTTP request string into an HttpRequest object
		static bool parseRequest(const std::string& raw_request, HttpRequest& request);
};
