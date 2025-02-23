#include "webserv.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include <memory>
#include <csignal>

#define BACKLOG 10 // how many pending connections queue will hold

ServerHandler::ServerHandler() {
	
	_servers.clear();
	_pollfd_list.clear();
	_ports.clear();
	_server_count = 0;
	_running = false;
	std::cout << "Constructor Size of pollfd list: " << _pollfd_list.size() << "\n";
}

ServerHandler::~ServerHandler() 
{
	this->cleanupServers();
	_servers.clear();
	_pollfd_list.clear();
	_clients.clear();
}

void	ServerHandler::error_and_exit(const char *msg)
{
	std::string errmsg = "Webserver: " + std::string(msg);
	perror(errmsg.c_str());
	exit(errno);
}

	/* Handles clean up of all servers */
void	ServerHandler::cleanupServers()
{
	for (auto& server : _servers) {
		std::vector<int> listen_sockfds = server->getListenSockfds();
		for (auto& sockfd : listen_sockfds) {
			close(sockfd);
		}
	}
}

void	ServerHandler::sendResponse(size_t& i)
{
	int clientFd = _pollfd_list[i].fd;
	
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		"Hello, world!";
	ssize_t bytes_sent;
	std::cout << "Sending back response: " << "\n";
	if ((bytes_sent = send(clientFd, response.c_str(), response.length(), 0)) == -1) {
		error_and_exit("Send");
	}
	std::cout << "Response sent successfully!" 
	<< " (sent " << bytes_sent << " bytes)"
	<< std::endl;
	if (_clients[clientFd].keep_alive == false)
		closeConnection(i);
}

/* 	One server config data instance is created. The config file is parsed in 
	the constructor and the appropriate values in the class instance is set. 
	Going through all serverBlock instances, a new shared pointer is made 
	for each virtual server with the appropriate configurations */
void	ServerHandler::setupServers(std::string path)
{
	for (const auto& [servName, config] : _config.getConfigBlocks()) 
	{
		_servers.emplace_back(std::make_shared<HttpServer>(config));
	}
	for (const auto& server : _servers)
	{
		server->setupAddrinfo();
	}
	_server_count = _servers.size();

	// HttpServer  serverInstance(current);
	// serverInstance.setPorts(current.getPorts());
	// serverInstance.setNumOfPorts(current.getNumOfPorts());
	// serverInstance.setupAddrinfo();
	// _servers.push_back(serverInstance);
}

void	ServerHandler::addConnection(size_t& i) {
	int clientFd;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr_in;
	struct pollfd new_pollfd;

	addrlen = sizeof(remoteaddr_in);
	clientFd = accept(_pollfd_list[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
	if (clientFd == -1) 
	{
		error_and_exit("Accept"); // log error
	}
	new_pollfd.fd = clientFd;
	new_pollfd.events = POLLIN | POLLOUT;
	new_pollfd.revents = 0;
	_pollfd_list.emplace_back(new_pollfd);
	t_client client = {};
	_clients[clientFd] = client;
}

void	ServerHandler::closeConnection(size_t& i) {
	int clientFd = _pollfd_list[i].fd;
	close(clientFd);
	_pollfd_list.erase(_pollfd_list.begin() + i);
	if (_clients[clientFd].request != nullptr) {
		delete _clients[clientFd].request;
		_clients[clientFd].request = nullptr;
	}
	_clients.erase(clientFd);
}

void	ServerHandler::readRequest(size_t& i)
{
	int clientFd = _pollfd_list[i].fd;
	int		numbytes;
	char	buf[1024];

	std::memset(buf, 0, 1024);
	if ((numbytes = recv(clientFd, buf, 1023, 0)) == -1) 
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cout << "No data available to read, try again later." << std::endl;
		} else {
			perror("recv");
			error_and_exit("Receive");
		}
	}
	std::cout << "Received request: " << buf << "\n";
}

void	ServerHandler::setPollList()
{
	size_t	i = 0;
	size_t	num_of_ports = getPortCount();

	std::cout << "Size of pollfd list: " << _pollfd_list.size() << "\n";
	_pollfd_list.resize(num_of_ports);
	std::cout << "Size of pollfd list: " << _pollfd_list.size() << "\n";
	for (auto& server : _servers)
	{		
		std::vector<int> listen_sockfds = server->getListenSockfds();
		for (size_t j = 0; j < listen_sockfds.size() ; j++) {
			_pollfd_list[i].fd = listen_sockfds[j];
			_pollfd_list[i].events = POLLIN;
			i++;
		}
	}
	printPollFds(); // for testing
}

void	ServerHandler::pollLoop()
{
	int			poll_count;
	
	setPollList();
	while (_running)
	{
		if ((poll_count = poll(_pollfd_list.data(), _pollfd_list.size(), -1)) == -1) {
			error_and_exit("Poll failed");
		}
		for(size_t i = 0; i < _pollfd_list.size(); i++)
		{
			if (_pollfd_list[i].revents & POLLIN) {
				if (_clients.find(_pollfd_list[i].fd) == _clients.end())
					addConnection(i);
				else
					readRequest(i);
			}
			else if (_pollfd_list[i].revents & POLLOUT && _clients[_pollfd_list[i].fd].requestReady == true)
				sendResponse(i);
		}
	}
}

void	ServerHandler::setupSockets()
{
	for (auto& server : _servers)
	{
		std::vector<struct addrinfo*> server_addresses = server->getAddrinfoVec();
		int	sockfd;

		for (auto& currentAddress : server_addresses)
		{
			struct addrinfo *p = currentAddress;
			int yes = 1;
			for (p = currentAddress; p != NULL; p = p->ai_next) 
			{
				if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
					error_and_exit("Socket");
					continue;
				}
				fcntl(sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
					error_and_exit("Setsockopt");
				}
				if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(sockfd);
					perror("Bind");
					continue;
				}
				server->addSockfd(sockfd);
				break;
			}
			if (p == NULL) {
				error_and_exit("Failed to bind");
			}
			if (listen(sockfd, BACKLOG) == -1) {
				error_and_exit("Listen");
			}
			freeaddrinfo(currentAddress);
		}
	}
}

void	ServerHandler::signalHandler(int signal) 
{
	// handle shutdown
	std::cout << " Ctrl + C signal received, shutting down\n";
	exit(signal);
}

void	ServerHandler::runServers()
{
	std::signal(SIGINT, ServerHandler::signalHandler);
	std::signal(SIGPIPE, SIG_IGN);

	_running = true;
	setupSockets();
	pollLoop();
	cleanupServers();
}