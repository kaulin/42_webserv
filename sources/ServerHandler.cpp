#include <memory>
#include <csignal>
#include <sys/socket.h>
#include <filesystem>
#include "ServerException.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include "RequestParser.hpp"
#include "RequestHandler.hpp"
#include "ResponseHandler.hpp"
#include "FileHandler.hpp"

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

void printClientInfo(const Client& client) {
	std::cout << "Client fd " << client.fd << ":\n";
	std::cout << "	bool cgiRequested: " << (client.cgiRequested ? "true" : "false") << "\n";
	std::cout << "	bool requestReady: " << (client.requestReady ? "true" : "false") << "\n";
	std::cout << "	bool responseReady: " << (client.responseReady ? "true" : "false") << "\n";
	std::cout << "	int responseCode: " << client.responseCode << "\n";
	std::cout << "\n";
}

void ServerHandler::resetClient(Client& client) {
	client.resourcePath = "";
	client.resourceInString = "";
	client.resourceOutString = "";
	client.resourceReadFd = -1;
	client.resourceBytesRead = 0;
	client.resourceWriteFd = -1;
	client.resourceBytesWritten = 0;
	client.cgiRequested = false;
	client.directoryListing = false;
	client.requestReady = false;
	client.responseReady = false;
	client.responseSent = false;
	client.responseCode = STATUS_OK;
	client.requestHandler->resetHandler();
}

void ServerHandler::removeFromPollList(int fdToRemove)
{
	std::cout << "Removes " << fdToRemove << " from poll list\n";
	auto it = _pollFds.begin();
	while (it != _pollFds.end())
	{
		if (it->fd == fdToRemove)
			it = _pollFds.erase(it);
		else
			++it;
	}
}

bool ServerHandler::checkTimeout(const Client& client)
{
	const std::time_t now = std::time(nullptr);
	const int timeout = 60;
	if (now - client.lastRequest > timeout)
		return false;
	return true;
}

void ServerHandler::closeConnection(size_t& i) 
{
	Client& client = *_clients[_pollFds[i].fd].get();
	int clientFd = _pollFds[i].fd;
	auto it = _requestFds.begin();
	while (it != _requestFds.end())
	{
		if (it->second == &client)
			removeResourceFd(it->first);
		else
			++it;
	}
	std::cout << "Client " << _pollFds[i].fd << " disconnected.\n";
	_pollFds.erase(_pollFds.begin() + i);
	_clients.erase(clientFd);
	close(clientFd);
	clientFd = -1;
	
}

void ServerHandler::checkClient(size_t& i) {
	Client& client = *_clients[_pollFds[i].fd].get();
	if (client.responseSent)
	{
		// if (client.keep_alive) Commented out to close all connections after response sent, as timeouts are not working
		// 	resetClient(client);
		// else
			closeConnection(i);
	}
	else if (!checkTimeout(client))
	{
		std::cout << "Client " << client.fd << " timed out, closing connection.\n";
		closeConnection(i);
	}
}

// Set pollin or pollout
void ServerHandler::addToPollList(int fd, PollType pollType)
{
	struct pollfd new_pollfd;
	
	new_pollfd.fd = fd;
	switch (pollType)
	{
	case SET_POLLBOTH:
		new_pollfd.events = POLLIN | POLLOUT;
		break;
	case SET_POLLIN:
		new_pollfd.events = POLLIN;
		break;
	case SET_POLLOUT:
		new_pollfd.events = POLLOUT;
		break;
	}
	new_pollfd.revents = 0;
	_pollFds.emplace_back(new_pollfd);
	std::cout << "Added " << new_pollfd.fd << " to poll list\n";
}

void ServerHandler::addResourceFd(Client& client) {
	// ADD RESOURCES OR PIPES TO POLL LOOP
	if (client.resourceReadFd != -1)
	{
		addToPollList(client.resourceReadFd, SET_POLLIN);
		_requestFds.emplace(client.resourceReadFd, &client);
	}
	if (client.resourceWriteFd != -1)
	{
		addToPollList(client.resourceWriteFd, SET_POLLOUT);
		_requestFds.emplace(client.resourceWriteFd, &client);
	}
}

void ServerHandler::removeResourceFd(int fd) {
	if (fd != -1) {
		removeFromPollList(fd);
		close(fd);
	}
	_requestFds.erase(fd);
}

void ServerHandler::addConnection(size_t& i) {
	int clientFd;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr_in;
	
	try {
		addrlen = sizeof(remoteaddr_in);
		clientFd = accept(_pollFds[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
		if (clientFd == -1)
			throw ServerException(STATUS_INTERNAL_ERROR);
		if (fcntl(clientFd, F_SETFL, O_NONBLOCK))
			throw ServerException(STATUS_INTERNAL_ERROR);
		addToPollList(clientFd, SET_POLLBOTH);
		_clients[clientFd] = std::make_unique<Client>();
		Client& client = *_clients[clientFd].get();
		client.serverConfig = _servers.at(i)->getServerConfig();
		client.fd = clientFd;
		client.keep_alive = false;
		client.requestHandler = std::make_unique<RequestHandler>(*_clients[clientFd].get());
		client.responseHandler = std::make_unique<ResponseHandler>(*_clients[clientFd].get());
		client.lastRequest = std::time(nullptr);
		resetClient(client);
		std::cout << "Client connected to server " << _servers.at(i)->getServerConfig()->port << " with fd " << client.fd << "\n";
	} catch (const ServerException& e) {
		// these should be logged, no response can be made, as there is no connection
		return;
	}
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
			if (!(_pollFds[i].revents & POLLIN) && !(_pollFds[i].revents & POLLOUT))
				continue;
			try {
				// Servers
				if (i < _serverCount) {
					if (_pollFds[i].revents & POLLIN)
						addConnection(i);
				}
				// Clients
				else if (_clients.find(_pollFds[i].fd) != _clients.end())
				{
					Client& client = *_clients[_pollFds[i].fd].get();

					if (_pollFds[i].revents & POLLIN)
					{
						client.requestHandler->handleRequest();
						client.lastRequest = std::time(nullptr);
						if (client.cgiRequested)
							client.resourceWriteFd = _CGIHandler.setupCGI(client);
						addResourceFd(client);
					}
					else if (_pollFds[i].revents & POLLOUT)
					{
						if (client.cgiRequested && !client.requestReady)
							_CGIHandler.runCGIScript(client);
						else if (client.requestReady)
						{
							client.responseHandler->formResponse();
							client.responseHandler->sendResponse();
						}
					}
					checkClient(i); // check timeouts
				}
				// Resources (pipes and files)
				else
				{
					if (_pollFds[i].revents & POLLIN)
						readFromFd(i);
					else if (_pollFds[i].revents & POLLOUT)
						writeToFd(i);
				}
			} catch (const ServerException& e) {
				handleServerException(e.statusCode(), i);
				//closeConnection(i);
			} catch (const std::exception& e) {
				std::cout << "Caught exception: " << e.what() << "\n";
			}
		}
	}
}

void	ServerHandler::handleServerException(int statusCode, size_t& fd)
{
	Client& client = (_clients.find(_pollFds[fd].fd) != _clients.end()) ? *_clients[_pollFds[fd].fd].get() : *_requestFds.at(_pollFds[fd].fd);
	if (client.responseCode == statusCode) {
		client.requestReady = true;
		client.resourceOutString = "";
		return;
	}
	client.responseCode = statusCode;
	client.responseReady = false;
	client.resourceOutString = "";
	if (client.resourceReadFd != -1) {
		removeResourceFd(client.resourceReadFd);
		close(client.resourceReadFd);
		client.resourceReadFd = -1;
	}
	auto it = client.serverConfig->error_pages.find(statusCode);
	if (it != client.serverConfig->error_pages.end()) {
		std::string path = ServerConfigData::getRoot(*client.serverConfig, "/") + it->second;
		//client.resourceReadFd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
		try {
			FileHandler::openForRead(client.resourceReadFd, path);
			addResourceFd(client);
		} catch (const ServerException& e) {
			// log error page missing
			// proceed with generating error page
			client.requestReady = true;
		}
	}
	else
		client.requestReady = true;
}

void	ServerHandler::readFromFd(size_t& i) {
	Client& client = *_requestFds.at(_pollFds[i].fd);
	int bytesRead;
	char buf[BUFFER_SIZE] = {};

	bytesRead = read(client.resourceReadFd, buf, BUFFER_SIZE);
	if (bytesRead <= 0) {
		throw ServerException(STATUS_INTERNAL_ERROR);
	}
	else {
		client.resourceInString.append(buf, bytesRead);
		client.resourceBytesRead += bytesRead;
		// std::cout << "Total bytes read: " << client.resourceBytesRead << "\n";
		if (bytesRead < BUFFER_SIZE)
		{
			client.requestReady = true;
			std::cout << "Client " << client.fd << " resource read from fd " << _pollFds[i].fd << "\n";
			removeResourceFd(client.resourceReadFd);
			close(client.resourceReadFd); // remove from pollFds and requestFds
			client.resourceReadFd = -1;
		}
		else
		{
			std::cout << "Client " << client.fd << " read " << bytesRead << " bytes from fd " << _pollFds[i].fd << ", continuing...\n";
		}
	}
}

void	ServerHandler::writeToFd(size_t& i) {
	Client& client = *_requestFds.at(_pollFds[i].fd);
	size_t bytesWritten;
	size_t bytesToWrite = client.resourceOutString.size() - client.resourceBytesWritten;
	if (bytesToWrite > BUFFER_SIZE)
		bytesToWrite = BUFFER_SIZE;

	bytesWritten = write(client.resourceWriteFd, client.resourceOutString.c_str() + client.resourceBytesWritten, bytesToWrite);
	if (bytesWritten <= 0)
		throw ServerException(STATUS_INTERNAL_ERROR);
	else {
		client.resourceBytesWritten += bytesWritten;
		if (client.resourceBytesWritten == client.resourceOutString.size())
		{
			client.requestReady = true;
			client.responseCode = STATUS_CREATED;
			std::cout << "Client " << client.fd << " resource written to fd " << _pollFds[i].fd << "\n";
			removeResourceFd(client.resourceWriteFd);
			close(client.resourceWriteFd); // remove from pollFds and requestFds
			client.resourceWriteFd = -1;
			client.requestHandler->handleRequest();
		}
		else
		{
			std::cout << "Client " << client.fd << " wrote " << bytesWritten << " bytes to fd " << _pollFds[i].fd << ", continuing...\n";
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
}
