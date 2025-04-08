#pragma once

#include <memory>
#include "HttpRequest.hpp"
#include "Client.hpp"

struct Client;

class RequestHandler
{
	private:
		std::unique_ptr<HttpRequest> _request;
		Client& _client;
		std::string _requestString;
		bool _readReady;
		void processRequest();
		void processGet();
		void processPost();
		void processDelete();
		void checkMethod() const; 
		// bool _chunkedRequest;
		// std::string _chunkedBodyString;
		// // bool _chunkedRequestReady;
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
