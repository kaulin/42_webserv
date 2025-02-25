#include "webserv.hpp"
#include "HttpServer.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(Config serverData)
{
	_addr_info = nullptr;
	_port = -1;
	_listen_sockfd = -1;
	_settings = serverData;
	_num_of_ports = serverData._ports.size();
	std::cout << "Created new virtual server instance\n";
} 

HttpServer::~HttpServer()
{
	close(_listen_sockfd);
}

void HttpServer::setupAddrinfo()
{
	struct addrinfo	hints;

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket

	int status = getaddrinfo(nullptr, _port.c_str(), &hints, &_addr_info);
	if (status != 0) {
		// log error
		throw std::runtime_error(gai_strerror(status));
	}
	struct addrinfo *p;
	for(p = _addr_info; p != NULL; p = p->ai_next) {
		void *addr;
		char ipstr[INET_ADDRSTRLEN];
		struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv->sin_addr);
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("Address: %s\n", ipstr);
	}
	std::cout << "-----finished setup addrinfo-----\n";
}

addrinfo* HttpServer::getAddrinfo() { return _addr_info; }

int HttpServer::getListenSockfd() { return _listen_sockfd; }

size_t HttpServer::getNumOfPorts() { return _num_of_ports; }

void HttpServer::setSocket(int sockfd) { _listen_sockfd = sockfd; }