#include <iostream>
#include <cassert>
#include "../includes/Request.hpp"

void testBasicRequest()
{
    std::string raw_request = "GET /index.html HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "User-Agent: CustomClient/1.0\r\n"
                              "\r\n"; // No body

    HttpRequestParser parser;
    HttpRequest request;

    // Test parsing the request
    bool success = parser.parseRequest(raw_request, request);
    assert(success);  // Ensure parsing was successful

    // Validate the parsed fields
    assert(request.method == "GET");
    assert(request.uri == "/index.html");
    assert(request.http_version == "HTTP/1.1");
    assert(request.headers.size() == 2);
    assert(request.headers["Host"] == "example.com");
    assert(request.headers["User-Agent"] == "CustomClient/1.0");
    assert(request.body.empty());  // No body

    std::cout << "Basic request test passed!" << std::endl;
}

void testRequestWithBody()
{
    std::string raw_request = "POST /submit HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "Content-Type: application/json\r\n"
                              "\r\n"
                              "{\"name\": \"John\", \"age\": 30}";

    HttpRequestParser parser;
    HttpRequest request;

    // Test parsing the request
    bool success = parser.parseRequest(raw_request, request);
    assert(success);  // Ensure parsing was successful

    // Validate the parsed fields
    assert(request.method == "POST");
    assert(request.uri == "/submit");
    assert(request.http_version == "HTTP/1.1");
    assert(request.headers.size() == 2);
    assert(request.headers["Host"] == "example.com");
    assert(request.headers["Content-Type"] == "application/json");
    assert(request.body == "{\"name\": \"John\", \"age\": 30}");

    std::cout << "Request with body test passed!" << std::endl;
}

void testRequestWithInvalidFormat()
{
    std::string raw_request = "GETindex.html HTTP/1.1\r\n"  // Missing space between "GET" and URI
                              "Host: example.com\r\n"
                              "\r\n";

    HttpRequestParser parser;
    HttpRequest request;

    // Test parsing the request
    bool success = parser.parseRequest(raw_request, request);
    assert(!success);  // Parsing should fail due to invalid format

    std::cout << "Invalid format test passed!" << std::endl;
}

void testEmptyRequest()
{
    std::string raw_request = "";  // Empty request

    HttpRequestParser parser;
    HttpRequest request;

    // Test parsing the request
    bool success = parser.parseRequest(raw_request, request);
    assert(!success);  // Parsing should fail due to empty request

    std::cout << "Empty request test passed!" << std::endl;
}

int main()
{
    testBasicRequest();
    testRequestWithBody();
    testRequestWithInvalidFormat();
    testEmptyRequest();

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
