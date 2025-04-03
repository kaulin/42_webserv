#include <memory>
#include <csignal>
#include <sys/socket.h>
#include "ServerException.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include "RequestParser.hpp"
#include "RequestHandler.hpp"
#include "ResponseHandler.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

std::vector<std::unique_ptr<HttpServer>> ServerHandler::_servers;

ServerHandler::ServerHandler(std::string path) : 
	_config(ServerConfigData(path)), _fileLogger("test_log.txt"), _consoleLogger(std::cout),
	_CGIHandler(CGIHandler())
{
	_serverCount = _config.getServerCount();
	_servers.reserve(_serverCount);
	_ports.reserve(_serverCount);
	_pollFds.reserve(_serverCount); // reserves space for ports
	_running = false;

	std::cout << "Constructor Size of pollfd list: " << _pollFds.capacity() << "\n";
}

ServerHandler::~ServerHandler() 
{
	_servers.clear();
	_pollFds.clear();

	std::cout << "Servers closed down\n";
}

/* 	One server config data instance is created. The config file is parsed in 
	the constructor and the appropriate values in the class instance is set. 
	Going through all serverBlock instances, a new shared pointer is made 
	for each virtual server with the appropriate configurations */
void	ServerHandler::setupServers()
{
	for (const auto& [servName, config] : _config.getConfigBlocks()) 
	{
		_servers.emplace_back(std::make_unique<HttpServer>(config));
	}
	for (const auto& server : _servers)
	{
		server->setupAddrinfo();
	}
	_serverCount = _servers.size();
}

void	ServerHandler::setPollList()
{
	size_t	i = 0;

	_pollFds.resize(_serverCount);
	for (auto& server : _servers)
	{		
		int listenSockfd = server->getListenSockfd();
		_pollFds[i].fd = listenSockfd;
		_pollFds[i].events = POLLIN;
		i++;
	}
	// for testing
	for (auto& poll_obj : _pollFds) 
	{
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
}

void	ServerHandler::error_and_exit(const char *msg)
{
	std::string errmsg = "Webserver: " + std::string(msg);
	perror(errmsg.c_str());
	exit(errno);
}

void	ServerHandler::sendResponse(size_t& i)
{
	int clientFd = _pollFds[i].fd;
	
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Content-Length: " + std::to_string(_clients[clientFd]->resourceString.length()) + "\r\n"
		"\r\n";
	response +=_clients[clientFd]->resourceString;
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

void	ServerHandler::addConnection(size_t& i) {
	int clientFd;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr_in;
	struct pollfd new_pollfd;

	try {
		addrlen = sizeof(remoteaddr_in);
		clientFd = accept(_pollFds[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
		if (clientFd == -1)
			throw std::runtime_error("Error: accept failed");
		if (fcntl(clientFd, F_SETFL, O_NONBLOCK))
			throw std::runtime_error("Error: fcntl failed");
		new_pollfd.fd = clientFd;
		new_pollfd.events = POLLIN | POLLOUT;
		new_pollfd.revents = 0;
		_pollFds.emplace_back(new_pollfd);
		_clients[clientFd] = std::make_unique<Client>();
		_clients[clientFd]->requestHandler = std::make_unique<RequestHandler>(*_clients[clientFd].get());
		_clients[clientFd]->serverConfig = _servers.at(i)->getServerConfig();
		_clients[clientFd]->fd = clientFd;
	} catch (std::exception& e) {
		// these should be logged, no response can be made, as there is no connection
	}
}

void	ServerHandler::closeConnection(size_t& i) {
	int clientFd = _pollFds[i].fd;
	close(clientFd);
	_pollFds.erase(_pollFds.begin() + i);
	_clients.erase(clientFd);
}

void	ServerHandler::pollLoop()
{
	int	poll_count;

	setPollList();
	while (_running)
	{
		if ((poll_count = poll(_pollFds.data(), _pollFds.size(), -1)) == -1)
			error_and_exit("Poll failed");
		for(size_t i = 0; i < _pollFds.size(); i++)
		{
			try {
				if (i < _serverCount)
				{
					addConnection(i);
					continue;
				}
				Client& client = *_clients[_pollFds[i].fd].get();
				if (_pollFds[i].revents & POLLIN)
						client.requestHandler->readRequest();
				else // if (_pollFds[i].revents & POLLIN)
				{
					if (client.cgiRequested)
					{
						_CGIHandler.setupCGI(client);
						_CGIHandler.runCGIScript(client);
						client.requestReady = true;
					}
					if (client.fileReadFd > 0)
						readFromFd(i);
					else if (client.fileWriteFd > 0)
						writeToFd(i);
					else if (client.responseReady == true)
					{
						sendResponse(i);
						// cleanClient();
					}
				}
				// checkTimeouts(i);
			} catch (const ServerException& e) {
				closeConnection(i);
			}
		}
	}
}

void	ServerHandler::readFromFd(size_t& i) {
	Client& client = *_clients[_pollFds[i].fd].get();
	int bytesRead;
	char buf[BUFFER_SIZE] = {};

	bytesRead = read(client.fileReadFd, buf, BUFFER_SIZE);
	if (bytesRead <= 0)
		throw std::runtime_error("Internal Server Error 500: read failed");
	else {
		client.resourceString.append(buf, bytesRead);
		client.fileTotalBytesRead += bytesRead;
		std::cout << "Total bytes read/file size: " << client.fileTotalBytesRead << "/" << client.fileSize << "\n";
		if (bytesRead < BUFFER_SIZE)
		{
			if (client.fileTotalBytesRead != client.fileSize)
				throw std::runtime_error("Internal Server Error 500: read failed");
			client.responseReady = true;
			std::cout << "Client [" << client.fd << "] response body read: " << client.resourceString << "\n";
			close(client.fileReadFd);
			client.fileReadFd = -1;
		}
		else
		{
			std::cout << "Client [" << client.fd << "] read " << bytesRead << " bytes from disk, continuing...\n";
			_pollFds[i].revents = POLL_OUT;
		}
	}
}

void	ServerHandler::writeToFd(size_t& i) {
	Client& client = *_clients[_pollFds[i].fd].get();
	const HttpRequest& request = client.requestHandler->getRequest();
	int bytesWritten;
	size_t leftToWrite = client.fileSize - client.fileTotalBytesWritten;
	size_t bytesToWrite = leftToWrite > BUFFER_SIZE ? BUFFER_SIZE : leftToWrite;
	char buf[BUFFER_SIZE] = {};
	request.body.copy(buf, BUFFER_SIZE, client.fileTotalBytesWritten);

	bytesWritten = write(client.fileWriteFd, buf, bytesToWrite);
	if (bytesWritten <= 0)
		throw std::runtime_error("Internal Server Error 500: write failed");
	else {
		client.fileTotalBytesWritten += bytesWritten;
		std::cout << "Total bytes written/file size: " << client.fileTotalBytesWritten << "/" << client.fileSize << "\n";
		if (bytesWritten < BUFFER_SIZE)
		{
			if (client.fileTotalBytesWritten != client.fileSize)
				throw std::runtime_error("Internal Server Error 500: write failed");
			client.responseReady = true;
			std::cout << "Client [" << client.fd << "] POST request resource saved to disk\n";
			close(client.fileWriteFd);
			client.fileWriteFd = -1;
		}
		else
		{
			std::cout << "Client [" << client.fd << "] wrote " << bytesWritten << " bytes to disk, continuing...\n";
			_pollFds[i].revents = POLL_OUT;
		}
	}
}

void	ServerHandler::signalHandler(int signal) 
{
	// handle shutdown
	std::cout << " Ctrl + C signal received, shutting down\n";
	_servers.clear();
	exit(signal);
}

void	ServerHandler::runServers()
{
	std::signal(SIGINT, ServerHandler::signalHandler);
	std::signal(SIGPIPE, SIG_IGN);

	_running = true;
	pollLoop();
	// cleanupServers();
}
