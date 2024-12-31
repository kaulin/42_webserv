#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigData.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(ServerConfigData server)
{
	_addr_info.clear();
	_ports.clear();
	_listen_sockfds.clear();
	_running = true;
	_num_of_ports = 1;
	_name = server.getName();
}

HttpServer::~HttpServer() {}

void	HttpServer::printListenFds()
{
	std::cout << "Host: " << _name << "\n";
	std::cout << "Number of ports: " << _num_of_ports << "\n";
	for (auto& curr : _listen_sockfds) {
		std::cout << "Listen sockfd: " << curr << "\n";
	}
	std::cout << "--------------------------\n";
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
		std::cout << "getting address info for port " << port.c_str() << "\n";
		if ((status = getaddrinfo("localhost", port.c_str(), &hints, &ai)) != 0) {
			throw std::runtime_error(gai_strerror(status));
		}
		_addr_info.push_back(ai);
		for(p = ai; p != NULL; p = p->ai_next) {
			void *addr;
			char ipstr[INET_ADDRSTRLEN];

			struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv->sin_addr);
			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
			printf("Address: %s\n", ipstr);
		}
	}
	std::cout << "-----finished setup addrinfo-----\n";
}

void HttpServer::setPorts(std::vector<std::string> ports)
{
	for (const auto& current : ports) {
		_ports.push_back(current);
	}
}

const std::vector<addrinfo*> HttpServer::getAddrinfoVec() { return _addr_info; }

std::string HttpServer::getName() { return _name; }

std::vector <int> HttpServer::getListenSockfds() { return _listen_sockfds; }

size_t HttpServer::getNumOfPorts() { return _num_of_ports; }

void HttpServer::setNumOfPorts(size_t num) { _num_of_ports = num; }

bool HttpServer::isRunning() { return _running; }

void HttpServer::setSockfd(int fd) 
{ 
	_listen_sockfds.push_back(fd);
	std::cout << "Added sockfd " << fd << " to server " << _name << "\n";
}
