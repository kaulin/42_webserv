// HttpHeader.hpp
#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <unordered_map>

// Temp placeholders
#define START_LINE "HTTP/1.1 200 OK\n"
#define BODY "<!DOCTYPE html><html><head><title>index.html</title></head><body><h1>Hello World!</h1><body></html>"

class HttpHeader
{
private:
	int _statusCode;
	std::deque<std::string> _headerKeys;
	std::unordered_map<std::string, std::string> _headers;
	std::string _body;
public:
	HttpHeader();
	~HttpHeader(); 
	void setStatusCode(const int statusCode);
	void addHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	const std::string HttpHeader::buildHeader() const;
	friend std::ostream& std::operator<<(std::ostream& os, const HttpHeader& obj);
};