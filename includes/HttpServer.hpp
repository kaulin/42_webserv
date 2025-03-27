#pragma once
#include "ServerConfigData.hpp"

class HttpServer
{
private:
		Config						_settings; // holds the config for each server
		std::string					_port;
		int							_sockFd;
public:
		HttpServer(Config data);
		~HttpServer();

		void			setupAddrinfo();
		void			setupSocket(struct addrinfo *ai);

		// get methods
		int					getListenSockfd();
};