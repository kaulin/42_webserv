#pragma once

#include <memory>
#include <vector>
#include "HttpRequest.hpp"
#include "Client.hpp"

struct Client;

enum ChunkParseState {
	READ_SIZE,
	READ_DATA,
	READ_CRLF
};

struct MultipartFormData {
	std::string filename;
	std::string contentType;
	std::string body;
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
		size_t	_partIndex;
		std::vector <MultipartFormData> _parts;
		void readRequest();
		void processRequest();
		void processGet();
		void processPost();
		void processDelete();
		void readHeaders();
		void readChunkedRequest();
		bool _isChunked;
		bool _chunkedBodyStarted;
		ChunkParseState _chunkState;
		size_t _expectedChunkSize = 0;
		bool isMultipartForm() const;
		void processMultipartForm();
	public:
		RequestHandler(Client& client);
		~RequestHandler();
		void resetHandler();
		void handleRequest();

		const HttpRequest& getRequest() const;
		// Not sure if the getters below are needed, as most of the work will be done with the whole struct from above
		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getUriQuery() const;
		const std::string& getUriPath() const;
		const std::string& getHttpVersion() const;
		const std::string& getBody() const;
		const std::vector <MultipartFormData>& getParts() const;

};
