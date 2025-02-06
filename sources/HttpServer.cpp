#include "webserv.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(ServerConfigData serverData)
{
	// Server data is an serverConfigData instance
	// Set up server instance using serverData
	// last line of defense to make sure ports and other invariants are set correctly
	FD_ZERO(&_addr_info);
	FD_ZERO(&_ports);
	FD_ZERO(&_listen_sockfds);
	FD_ZERO(&_num_of_ports);
	FD_ZERO(&_name);
}

HttpServer::~HttpServer()
{
	for (auto& listen_sockfd : _listen_sockfds) {
		close(listen_sockfd);
	}
	_addr_info.clear();
	_ports.clear();
	_listen_sockfds.clear();
}

void HttpServer::setupAddrinfo()
{
	int	status;
	struct addrinfo	hints;

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket

	for (auto& port : _ports)
	{
		struct addrinfo* ai;
		struct addrinfo *p;
		if ((status = getaddrinfo("localhost", port.c_str(), &hints, &ai)) != 0) {
			throw std::runtime_error(gai_strerror(status));
		}
		for(p = ai; p != NULL; p = p->ai_next) {
			void *addr;
			char ipstr[INET_ADDRSTRLEN];
			struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv->sin_addr);
			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
			printf("Address: %s\n", ipstr);
		}
		_addr_info.emplace_back(ai);
	}
	std::cout << "-----finished setup addrinfo-----\n";
}

void HttpServer::setPorts(std::vector<std::string> ports)
{
	for (const auto& current : ports) {
		_ports.emplace_back(current);
	}
}

void HttpServer::setName(std::string name) { _name = name; }

const std::vector<addrinfo*> HttpServer::getAddrinfoVec() { return _addr_info; }

std::string HttpServer::getName() { return _name; }

std::vector <int> HttpServer::getListenSockfds() { return _listen_sockfds; }

size_t HttpServer::getNumOfPorts() { return _num_of_ports; }

void HttpServer::setNumOfPorts(size_t num) { _num_of_ports = num; }

void HttpServer::addSockfd(int fd) { _listen_sockfds.emplace_back(fd); }
