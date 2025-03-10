#include "webserv.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include <memory>
#include <csignal>

#define BACKLOG 10 // how many pending connections queue will hold

ServerHandler::ServerHandler(std::string path) : 
	_config(ServerConfigData(path)), _fileLogger("test_log.txt"), _consoleLogger(std::cout)
{
	_serverCount = _config.getServerCount();
	_servers.reserve(_serverCount);
	_ports.reserve(_config.getServerCount());
	_pollFds.reserve(_config.getServerCount()); // reserves space for ports
	_running = false;
	std::cout << "Constructor Size of pollfd list: " << _pollFds.size() << "\n";
}

ServerHandler::~ServerHandler() 
{
	this->cleanupServers();
	_servers.clear();
	_pollFds.clear();

	std::cout << "Servers closed down\n";
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
		int listen_sockfd = server->getListenSockfd();
		close(listen_sockfd);
	}
}

void	ServerHandler::sendResponse(size_t& i)
{
	int clientFd = _pollFds[i].fd;
	
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
void	ServerHandler::setupServers()
{
	for (const auto& [servName, config] : _config.getConfigBlocks()) 
	{
		_servers.emplace_back(std::make_shared<HttpServer>(config));
	}
	for (const auto& server : _servers)
	{
		server->setupAddrinfo();
	}
	_serverCount = _servers.size();

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

	try {
		addrlen = sizeof(remoteaddr_in);
		clientFd = accept(_pollFds[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
		if (clientFd == -1)
			throw std::runtime_error("Internal Server Error 500: accept failed");
		if (fcntl(clientFd, F_SETFL, O_NONBLOCK))
			throw std::runtime_error("Internal Server Error 500: fcntl failed");
		new_pollfd.fd = clientFd;
		new_pollfd.events = POLLIN | POLLOUT;
		new_pollfd.revents = 0;
		_pollFds.emplace_back(new_pollfd);
		t_client client = {};
		_clients[clientFd] = client;
	} catch (std::exception& e) {
		_clients[clientFd].responseCode = 500;
		_clients[clientFd].responseReady = true;
	}
}

void	ServerHandler::closeConnection(size_t& i) {
	int clientFd = _pollFds[i].fd;
	close(clientFd);
	_pollFds.erase(_pollFds.begin() + i);
	if (_clients[clientFd].request != nullptr) {
		delete _clients[clientFd].request;
		_clients[clientFd].request = nullptr;
	}
	_clients.erase(clientFd);
}

void	ServerHandler::readRequest(size_t& i)
{
	int clientFd = _pollFds[i].fd;
	int receivedBytes;
	char buf[1024] = {};

	try {
		receivedBytes = recv(clientFd, buf, 1023, 0);
		if (receivedBytes < 0)
			throw std::runtime_error("Internal Server Error 500: recv failed");
		else if (receivedBytes == 0)
			closeConnection(i);
		else {
			_clients[clientFd].requestString.append(buf, receivedBytes);
			if (receivedBytes < 1024) {
				_clients[clientFd].requestReady = true;
				std::cout << "Client [" << clientFd << "] request ready: " << _clients[clientFd].requestString << "\n";
			}
			else
				std::cout << "Received request portion: " << buf << "\n";
		}
	} catch (std::exception &e) {
		closeConnection(i);
		throw ;
	}
}

void	ServerHandler::setPollList()
{
	size_t	i = 0;

	_pollFds.resize(_serverCount);
	for (auto& server : _servers)
	{		
		int listen_sockfd = server->getListenSockfd();
		_pollFds[i].fd = listen_sockfd;
		_pollFds[i].events = POLLIN;
		i++;
	}
	for (auto& poll_obj : _pollFds) 
	{
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
}

void	ServerHandler::pollLoop()
{
	int			poll_count;
	
	setPollList();
	while (_running)
	{
		try {
			if ((poll_count = poll(_pollFds.data(), _pollFds.size(), -1)) == -1) {
				error_and_exit("Poll failed");
			}
			for(size_t i = 0; i < _pollFds.size(); i++)
			{
				if (_pollFds[i].revents & POLLIN) {
					if (_clients.find(_pollFds[i].fd) == _clients.end())
					{
						addConnection(i);
					}
					else
						readRequest(i);
				}
				else if (_pollFds[i].revents & POLLOUT && _clients[_pollFds[i].fd].requestReady == true)
					sendResponse(i);
			}
		} catch (std::exception& e) {
// Errors should be handled lower down and rethrown to be caught here.
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
	pollLoop();
	cleanupServers();
}

void	ServerHandler::printPollFds()
{
	for (auto& poll_obj : _pollFds) 
	{
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
}