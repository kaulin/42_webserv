#pragma once
#include "webserv.hpp"
#include "Request.hpp"
#include "CGIHandler.hpp"

class HttpServer;

#define BUFFER_SIZE 1024

typedef struct s_client {
	int								fd;
	std::string						requestString;
	bool							requestReady;
	std::unique_ptr<HttpRequest>	request;
	int								fileSize;
	int								fileReadFd;
	int								fileTotalBytesRead;
	int								fileWriteFd;
	int								fileTotalBytesWritten;
	std::string						responseBodyString;
	int								responseCode;
	bool							responseReady;
	std::time_t						lastRequest;
	bool							keep_alive;

	// For CGI handling
	std::shared_ptr<CGIHandler>	clientCGI;
} t_client;

class ServerHandler
{
private:
	std::vector<std::shared_ptr<HttpServer>>			_servers;
	size_t					 							_serverCount;
	std::vector<int>									_ports;
	std::unordered_map<int, std::unique_ptr<t_client>>	_clients;
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
