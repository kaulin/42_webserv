#include "webserv.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include "Request.hpp"
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
	if (_clients[clientFd]->keep_alive == false)
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
		_clients[clientFd] = std::make_unique<t_client>();
	} catch (std::exception& e) {
		_clients[clientFd]->responseCode = 500;
		_clients[clientFd]->responseReady = true;
	}
}

void	ServerHandler::closeConnection(size_t& i) {
	int clientFd = _pollFds[i].fd;
	close(clientFd);
	_pollFds.erase(_pollFds.begin() + i);
	if (_clients[clientFd]->request != nullptr) {
		_clients[clientFd]->request = nullptr;
	}
	_clients.erase(clientFd);
}

void	ServerHandler::readRequest(size_t& i)
{
	int clientFd = _pollFds[i].fd;
	int receivedBytes;
	char buf[BUFFER_SIZE] = {};

	try {
		receivedBytes = recv(clientFd, buf, BUFFER_SIZE, 0);
		if (receivedBytes < 0)
			throw std::runtime_error("Internal Server Error 500: recv failed");
		else if (receivedBytes == 0)
			closeConnection(i);
		else {
			_clients[clientFd]->requestString.append(buf, receivedBytes);
			if (receivedBytes < BUFFER_SIZE) { // whole request read
				_clients[clientFd]->requestReady = true;
				std::cout << "Client [" << clientFd << "] request ready: " << _clients[clientFd]->requestString << "\n";
				processRequest(i);
			}
			else // more incoming
				std::cout << "Received request portion: " << buf << "\n";
		}
	} catch (std::exception &e) {
		closeConnection(i);
		throw;
	}
}

void	ServerHandler::processRequest(size_t& i) 
{
	t_client& client = *_clients[_pollFds[i].fd].get();
	HttpRequestParser requestParser;
	client.request = std::make_unique<HttpRequest>();
	
	try {
		requestParser.parseRequest(client.requestString, *client.request.get());
	} catch (std::exception& e) {
		throw;
	}
	
	std::cout << "Client " << client.fd << " request method " << client.request->method << " and URI: " << client.request->uri << "\n";

	if (client.request->method == "POST")
	{
		std::string path = "/home/jajuntti/42/webserv/file-handling/var/www/uploads" + client.request->uri;
		client.fileSize = client.request->body.size();
		client.fileWriteFd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
	}
	// if (client.request->headers.find("Connection") != client.request->headers.end() 
	// 	&& (client.request->headers["Connection"] == "keep-alive" 
	// 	|| client.request->headers["Connection"] == "Keep-Alive"))
	// 	client.keep_alive = true;
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
			if ((poll_count = poll(_pollFds.data(), _pollFds.size(), -1)) == -1)
				error_and_exit("Poll failed");
			for(size_t i = 0; i < _pollFds.size(); i++)
			{
				if (_pollFds[i].revents & POLLIN)
				{
					if (_clients.find(_pollFds[i].fd) == _clients.end())
						addConnection(i);
					else if (_clients[_pollFds[i].fd]->fileReadFd > 0)
						writeToFd(i);
					else
						readRequest(i);
				}
				else if (_pollFds[i].revents & POLLOUT)
				{
					if (_clients[_pollFds[i].fd]->fileWriteFd > 0)
						writeToFd(i);
					else if (_clients[_pollFds[i].fd]->requestReady == true)
						sendResponse(i);
				}
			}
		} catch (std::exception& e) {
// Errors should be handled lower down and rethrown to be caught here.
		}
	}
}

void	ServerHandler::readFromFd(size_t& i) {
	t_client& client = *_clients[_pollFds[i].fd].get();
	int bytesRead;
	char buf[BUFFER_SIZE] = {};

	try {
		bytesRead = read(client.fileReadFd, buf, BUFFER_SIZE);
		if (bytesRead <= 0)
			throw std::runtime_error("Internal Server Error 500: read failed");
		else {
			client.responseString.append(buf, bytesRead);
			if (bytesRead < BUFFER_SIZE)
			{
				client.responseReady = true;
				std::cout << "Client [" << client.fd << "] response body read: " << client.responseString << "\n";
				close(client.fileReadFd);
				client.fileReadFd = -1;
			}
			else
				_pollFds[i].events = POLL_IN;
		}
	} catch (std::exception &e) {
		throw;
	}

}

void	ServerHandler::writeToFd(size_t& i) {
	t_client& client = *_clients[_pollFds[i].fd].get();
	int bytesWritten;
	char buf[BUFFER_SIZE] = {};
	client.request->body.copy(buf, client.fileSize);

	try {
		bytesWritten = write(client.fileWriteFd, buf, client.fileSize);
		if (bytesWritten <= 0)
			throw std::runtime_error("Internal Server Error 500: write failed");
		else {
			if (bytesWritten < BUFFER_SIZE)
			{
				client.responseReady = true;
				std::cout << "Client [" << client.fd << "] POST request resource saved to disk\n";
				close(client.fileWriteFd);
				client.fileWriteFd = -1;
			}
			else
			{
				std::cout << "Client [" << client.fd << "] wrote " << bytesWritten << " bytes to disk, continuing...\n";
				_pollFds[i].events = POLL_OUT;
			}
		}
	} catch (std::exception &e) {
		throw e;
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