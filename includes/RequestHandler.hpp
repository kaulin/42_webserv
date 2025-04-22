#pragma once

#include <memory>
#include "HttpRequest.hpp"
#include "Client.hpp"

struct Client;

enum ChunkParseState {
    READ_SIZE,
    READ_DATA,
    READ_CRLF
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
		void processRequest();
		void processGet();
		void processPost();
		void processDelete();
		void searchForChunked();
		void readChunkedRequest();
		bool _isChunked;
		bool _chunkedBodyStarted;
		ChunkParseState _chunkState;
		size_t _expectedChunkSize = 0;
	public:
		RequestHandler(Client& client);
		~RequestHandler();
		void resetHandler();
		void readRequest();

		const HttpRequest& getRequest() const;
		// Not sure if the getters below are needed, as most of the work will be done with the whole struct from above
		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getUriQuery() const;
		const std::string& getUriPath() const;
		const std::string& getHttpVersion() const;
		const std::string& getBody() const;

};
