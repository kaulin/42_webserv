#pragma once

#include <string>
#include <unordered_map>
#include "HttpRequest.hpp"
#include "RequestHandler.hpp"

// The http request parser class
class RequestParser
{
	private:
		RequestParser() = default;
		~RequestParser() = default;
		static bool parseRequestLine(const std::string& request_line, HttpRequest& request);	// Function to parse the request line
		static bool parseHeaders(const std::string& headers_part, HttpRequest& request);		// Function to parse headers (key: value)
		static void parseBody(const std::string& body_part, HttpRequest& request);				// Function to parse the body
		static void parseMultipartHeaders(std::map<std::string, std::string>& headerMap, const std::string& headers);
		static std::string getFilename(const std::string& contentDisposition);
	public:
		// Parses raw HTTP request string into an HttpRequest object
		static bool parseRequest(const std::string& raw_request, HttpRequest& request);
		static std::string parseChunkedBody(const std::string& chunked);						// Function to parse a chunked body
		static void parseMultipart(const std::string& boundary, const std::string& body, std::vector <MultipartFormData>& parts);
};
