#pragma once
#include <ctime>
#include <memory>
#include <poll.h>
#include "ServerConfigData.hpp"
#include "Client.hpp"
#include "RequestHandler.hpp"
#include "CGIHandler.hpp"
#include "Logger.hpp"

class HttpServer;
// class CGIHandler;

#define BUFFER_SIZE 1024
#define DEFAULT_CONFIG_FILE "config/default.conf"
#define BACKLOG 10 // how many pending connections queue will hold

class ServerHandler
{
private:
	static std::vector<std::unique_ptr<HttpServer>>		_servers;
	size_t					 							_serverCount;
	std::vector<int>									_ports;
	std::unordered_map<int, std::unique_ptr<Client>>	_clients;
	std::vector<struct pollfd>							_pollFds;
	bool												_running;
	ServerConfigData									_config;
	Logger												_fileLogger;
	Logger												_consoleLogger;
	CGIHandler											_CGIHandler;
	void	readFromFd(size_t& i);
	void	writeToFd(size_t& i);
public:
	ServerHandler(std::string path);
	~ServerHandler();

	void		runServers();
	void		setupServers();
	void		pollLoop();
	void		setPollList();
	void		error_and_exit(const char *msg);
	void		addConnection(size_t& i);
	void		closeConnection(size_t& i);
	void		readRequest(size_t& i);
	void		processRequest(size_t& i);
	void		sendResponse(size_t& i);
	void		cleanupServers();
	
	static void	signalHandler(int);

	// Helper functions for debugging
	// void	*get_in_addr(struct sockaddr *sa);
	// void	printServerData();
};
