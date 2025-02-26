#include "webserv.hpp"
#include "HttpServer.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(Config serverData)
{
	_port = serverData._port;
	_sockFd = -1;
	_settings = serverData;
	std::cout << "Created new virtual server instance\n";
} 

HttpServer::~HttpServer()
{
	close(_sockFd);
}

void HttpServer::setupSocket(struct addrinfo *ai)
{
	struct addrinfo *p = ai;
	int yes = 1;
	
	for (p = ai; p != NULL; p = p->ai_next)
	{
		if ((_sockFd = socket(AF_INET, p->ai_socktype, p->ai_protocol)) == -1) {
			throw ("Socket");
		}
		// sets the socket to non-blocking
		if (fcntl(_sockFd, F_SETFL, O_NONBLOCK) == -1) {
			close(_sockFd);
			throw std::runtime_error("fcntl");
		}
		if (setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			throw std::runtime_error("Setsockopt");
		}
		if (bind(_sockFd, p->ai_addr, p->ai_addrlen) == -1) {
			close(_sockFd);
			throw std::runtime_error("Bind");
		}	
		break;
	}
	if (p == NULL) {
		throw std::runtime_error("Failed to bind");
	}
	if (listen(_sockFd, BACKLOG) == -1) {
		throw std::runtime_error("Listen");
	}
	freeaddrinfo(ai);
}

void HttpServer::setupAddrinfo()
{
	struct addrinfo	hints;

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket
	
	struct addrinfo* ai;
	int status = getaddrinfo(nullptr, _port.c_str(), &hints, &ai);
	if (status != 0) {
		// log error
		throw std::runtime_error(gai_strerror(status));
	}
	struct addrinfo *p;
	for(p = ai; p != NULL; p = p->ai_next) {
		void *addr;
		char ipstr[INET_ADDRSTRLEN];
		struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv->sin_addr);
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("Address: %s\n", ipstr);
	}
	setupSocket(ai);
	std::cout << "-----finished setup addrinfo & socket for server-----\n";
}

int HttpServer::getListenSockfd() { return _sockFd; }
