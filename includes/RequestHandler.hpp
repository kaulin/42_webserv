#pragma once

#include <memory>
#include <vector>
#include "HttpRequest.hpp"
#include "Client.hpp"

struct Client;

enum ChunkParseState {
	SIZE,
	DATA,
	CRLF
};

struct MultipartFormData {
	std::string filename;
	std::string contentType;
	std::string content;
};

class RequestHandler
{
	private:
		std::unique_ptr<HttpRequest> _request;
		Client& _client;
		std::string _requestString;
		std::string _headerPart;
		std::string _chunkBuffer;
		std::string _decodedBody;
		bool _headersRead;
		bool _readReady;
		bool _multipart;
		size_t _expectedContentLength;
		size_t _totalReceivedLength;
		size_t _partIndex;
		std::vector <MultipartFormData> _parts;
		void processRequest();
		void processGet();
		void processPost();
		void processDelete();
		void handleHeaders();
		void handleChunkedRequest();
		void setContentLength();
		bool checkRedirect();
		bool _isChunked;
		bool _chunkedBodyStarted;
		ChunkParseState _chunkState;
		size_t _expectedChunkSize = 0;
		void processMultipartForm();
		void checkContentType() const;
		void checkAcceptType() const;
		void checkContentLength() const;
	public:
		RequestHandler(Client& client);
		~RequestHandler();
		void resetHandler();
		void handleRequest();
		void readRequest();

		const HttpRequest& getRequest() const;
		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getUriQuery() const;
		const std::string& getUriPath() const;
		const std::string& getHttpVersion() const;
		const std::string& getBody() const;
		const std::vector <MultipartFormData>& getParts() const;
		bool getReadReady() const;
};
