#include <csignal>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sys/socket.h>
#include "FileHandler.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "RequestParser.hpp"
#include "RequestHandler.hpp"
#include "ResponseHandler.hpp"
#include "ServerException.hpp"
#include "ServerHandler.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

bool ServerHandler::_sigintReceived = false;

void	ServerHandler::signalHandler(int signal) 
{
	(void)signal;
	std::cout << std::endl;
	Logger::log(Logger::OK, "SIGINT received");
	_sigintReceived = true;
}

ServerHandler::ServerHandler(std::string path) : 
	_config(ServerConfigData(path)),
	_CGIHandler(CGIHandler())
{
	_serverCount = _config.getServerCount();
	_servers.reserve(_serverCount);
	_ports.reserve(_serverCount);
	_pollFds.reserve(_serverCount);
}

ServerHandler::~ServerHandler() 
{
	_servers.clear();
	_pollFds.clear();
	_newPollFds.clear();
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
		Logger::log(Logger::OK, "Server " + server->getServerConfig()->host + ":" + server->getServerConfig()->port + " polling on socket " + std::to_string(listenSockfd));
		i++;
	}
}

void ServerHandler::resetClient(Client& client) {
	client.resourcePath = "";
	client.resourceInString = "";
	client.resourceOutString = "";
	client.resourceReadFd = -1;
	client.resourceBytesRead = 0;
	client.resourceWriteFd = -1;
	client.resourceBytesWritten = 0;
	client.keepAlive = true;
	client.cgiRequested = false;
	client.cgiStatus = -1;
	client.directoryListing = false;
	client.requestReady = false;
	client.responseReady = false;
	client.responseSent = false;
	client.responseCode = STATUS_OK;
	client.connectionState = ACTIVE;
	client.requestHandler->resetHandler();
	client.responseHandler->resetHandler();
}

bool ServerHandler::checkTimeout(const Client& client)
{
	const std::time_t now = std::time(nullptr);
	const int timeout = (client.serverConfig->timeout != 0) ? client.serverConfig->timeout : 7;
	if (now - client.lastActivity > timeout)
		return false;
	return true;
}

void ServerHandler::checkClients()
{
	for (size_t i = 0; i < _pollFds.size() ; i++)
	{
		if (_clients.empty())
			return;
		auto it = _clients.find(_pollFds[i].fd);
		if (it != _clients.end())
		{
			Client& client = *it->second.get();
			if (!checkTimeout(client))
			{
				Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " timed out, disconnecting");
				closeConnection(i);
			}
			else if (client.connectionState != DRAIN && client.responseSent && !client.keepAlive)
			{
				Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " connection disconnected by server");
				closeConnection(i);
			}
			else if (client.connectionState == CLOSE)
			{
				Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " connection disconnected by client");
				closeConnection(i);
			}
			else if (client.responseSent)
				resetClient(client);
		}
	}
	updatePollList();
}

void ServerHandler::closeConnection(size_t& i) 
{
	Client& client = *_clients[_pollFds[i].fd].get();
	_CGIHandler.killCGIProcess(client);
	int clientFd = _pollFds[i].fd;
	auto it = _resourceFds.begin();
	while (it != _resourceFds.end() && !_resourceFds.empty())
	{
		if (it->second == &client)
			removeResourceFd(it->first);
		else
			++it;
	}
	_clients.erase(clientFd);
	_fdsToDrop.push_back(clientFd);
	close(clientFd);
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
	_newPollFds.emplace_back(new_pollfd);
}

void ServerHandler::removeFromPollList(int fdToRemove)
{
	for (auto it = _pollFds.begin() ; it != _pollFds.end(); it++)
	{
		if (it->fd == fdToRemove)
		{
			_pollFds.erase(it);
			return;
		}
	}
}

void ServerHandler::updatePollList()
{
	if (_sigintReceived)
		return;
	_pollFds.insert(_pollFds.end(), _newPollFds.begin(), _newPollFds.end());
	_newPollFds.clear();
	while (!_fdsToDrop.empty())
	{
		removeFromPollList(_fdsToDrop.back());
		_fdsToDrop.pop_back();
	}
}

void ServerHandler::addResourceFd(Client& client) {
	// ADD RESOURCES OR PIPES TO POLL LOOP
	if (client.resourceReadFd != -1)
	{
		addToPollList(client.resourceReadFd, SET_POLLIN);
		_resourceFds.emplace(client.resourceReadFd, &client);
	}
	if (client.resourceWriteFd != -1)
	{
		addToPollList(client.resourceWriteFd, SET_POLLOUT);
		_resourceFds.emplace(client.resourceWriteFd, &client);
	}
}

void ServerHandler::removeResourceFd(int fd) {
	if (fd != -1) {
		_fdsToDrop.push_back(fd);
		close(fd);
	}
	_resourceFds.erase(fd);
}

void ServerHandler::addConnection(size_t& i) {
	int clientFd;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr_in;

	addrlen = sizeof(remoteaddr_in);
	clientFd = accept(_pollFds[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
	if (clientFd == -1)
	{
		Logger::log(Logger::ERROR, "accept failed on new connection to " + _servers.at(i)->getServerConfig()->host + _servers.at(i)->getServerConfig()->port );
		return;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		Logger::log(Logger::ERROR, "fcntl failed on new connection to " + _servers.at(i)->getServerConfig()->host + _servers.at(i)->getServerConfig()->port );
		close(clientFd);
		return;
	}
	addToPollList(clientFd, SET_POLLBOTH);
	_clients[clientFd] = std::make_unique<Client>();
	Client& client = *_clients[clientFd].get();
	client.serverConfig = _servers.at(i)->getServerConfig();
	client.fd = clientFd;
	client.keepAlive = false;
	client.requestHandler = std::make_unique<RequestHandler>(*_clients[clientFd].get());
	client.responseHandler = std::make_unique<ResponseHandler>(*_clients[clientFd].get());
	client.lastActivity = std::time(nullptr);
	resetClient(client);
	
	Logger::log(Logger::OK, "Client " + std::to_string(client.fd) + " connected to server " + _servers.at(i)->getServerConfig()->host + ":" +  _servers.at(i)->getServerConfig()->port );
}

void	ServerHandler::pollLoop()
{
	int	pollCount;

	std::signal(SIGINT, ServerHandler::signalHandler);
	std::signal(SIGPIPE, SIG_IGN);
	setPollList();
	while (!_sigintReceived)
	{
		try {
			pollCount = poll(_pollFds.data(), _pollFds.size(), -1);
			if (_sigintReceived)
					break;
			if (pollCount == -1)
			{
				if (errno == EINTR)
					continue;
				Logger::log(Logger::ERROR, "poll failed: " + std::string(std::strerror(errno)));
				break;
			}
			checkClients();
			for(size_t i = 0; i < _pollFds.size(); i++)
			{
				try {
					if (_pollFds[i].revents & POLLIN)
					{
						if (i < _serverCount) // Servers in
							addConnection(i);
						else if (_clients.find(_pollFds[i].fd) != _clients.end()) // Clients in
						{
							Client& client = *_clients[_pollFds[i].fd].get();
							client.requestHandler->handleRequest();
							if (client.cgiRequested)
								_CGIHandler.handleCGI(client);
							if (client.responseCode == STATUS_OK)
								addResourceFd(client);
						}
						else // Resource in
							readFromFd(i);
					}
					else if (_pollFds[i].revents & POLLOUT)
					{
						if (_clients.find(_pollFds[i].fd) != _clients.end()) // Clients out
						{
							Client& client = *_clients[_pollFds[i].fd].get();
							client.responseHandler->formResponse();
							client.responseHandler->sendResponse();
						}
						else // Resource out
							writeToFd(i);
					} 
				} catch (const ServerException& e) {
					if (_sigintReceived)
						break;
					handleServerException(e.statusCode(), i);
					break;
				}
			}
			updatePollList();
		} catch (const std::exception& e) {
			if (_sigintReceived)
				break;
			Logger::log(Logger::ERROR, "Caught exception: " + std::string(e.what()));
			continue;
		}
	}
	for (pollfd p : _pollFds)
		close(p.fd);
	for (pollfd p : _newPollFds)
		close(p.fd);
}

void	ServerHandler::handleServerException(int statusCode, size_t& i)
{
	// Set client from either _clients struct or from requestFds struct.
	Client& client = (_clients.find(_pollFds[i].fd) != _clients.end()) ? *_clients[_pollFds[i].fd].get() : *_resourceFds.at(_pollFds[i].fd);

	// If connection has been severed by client or recv or send returns -1
	if (statusCode == STATUS_DISCONNECTED || statusCode == STATUS_RECV_ERROR || statusCode == STATUS_SEND_ERROR) {
		client.connectionState = CLOSE;
		return;
	}

	// If whole request has not been read before error occurs, connection must be closed after draining socket and sending error response
	if (!client.requestHandler->getReadReady()) {
		client.connectionState = DRAIN;
		client.keepAlive = false;
	}

	// If error occurs during error page creation, form response without error page to prevent looping
	if (client.responseCode == statusCode) {
		client.requestReady = true;
		client.resourceOutString = "";
		return;
	}

	client.responseCode = statusCode;
	client.requestReady = false;
	client.responseReady = false;
	client.resourceOutString = "";
	if (client.resourceReadFd != -1) {
		removeResourceFd(client.resourceReadFd);
		client.resourceReadFd = -1;
	}
	auto it = client.serverConfig->error_pages.find(statusCode);
	if (it != client.serverConfig->error_pages.end()) {
		std::string path = ServerConfigData::getRoot(*client.serverConfig, "/") + it->second;
		try {
			FileHandler::openForRead(client.resourceReadFd, path);
			addResourceFd(client);
		} catch (const ServerException& e) { // if an error occurs when opening error page resource
			Logger::log(Logger::ERROR, "error page " + path + " not found, generating response");
			client.requestReady = true;
		}
	}
	else
		client.requestReady = true;
}

void	ServerHandler::readFromFd(size_t& i) {
	if (_resourceFds.find(_pollFds[i].fd) == _resourceFds.end())
		return;
	Client& client = *_resourceFds.at(_pollFds[i].fd);
	ssize_t bytesRead;
	char buf[BUFFER_SIZE] = {};

	bytesRead = read(client.resourceReadFd, buf, BUFFER_SIZE);
	if (bytesRead <= 0)
		throw ServerException(STATUS_INTERNAL_ERROR);
	client.resourceInString.append(buf, bytesRead);
	client.resourceBytesRead += bytesRead;
	if (bytesRead < BUFFER_SIZE)
	{
		client.requestReady = true;
		if (client.cgiRequested)
			client.cgiStatus = _CGIHandler.cleanupCGI(client);
		removeResourceFd(client.resourceReadFd);
		client.resourceReadFd = -1;
	}
}

void	ServerHandler::writeToFd(size_t& i) {
	if (_resourceFds.find(_pollFds[i].fd) == _resourceFds.end())
		return;
	Client& client = *_resourceFds.at(_pollFds[i].fd);
	ssize_t bytesWritten;
	size_t bytesToWrite = client.resourceOutString.size() - client.resourceBytesWritten;
	if (bytesToWrite > BUFFER_SIZE)
		bytesToWrite = BUFFER_SIZE;

	bytesWritten = write(client.resourceWriteFd, client.resourceOutString.c_str() + client.resourceBytesWritten, bytesToWrite);
	if (bytesWritten <= 0 && bytesToWrite != 0)
		throw ServerException(STATUS_INTERNAL_ERROR);
	client.resourceBytesWritten += bytesWritten;
	if (client.resourceBytesWritten == client.resourceOutString.size())
	{
		client.responseCode = STATUS_CREATED;
		removeResourceFd(client.resourceWriteFd);
		client.resourceWriteFd = -1;
		client.resourceBytesWritten = 0;
		client.requestHandler->handleRequest();
		if (client.cgiRequested) {
			_CGIHandler.handleCGI(client);
		}
		addResourceFd(client);
	}
}

void	ServerHandler::runServers()
{
	Logger::start("Starting servers");
	pollLoop();
	Logger::stop("Shutting down servers");
	_resourceFds.clear();
	_clients.clear();
	_servers.clear();
}
