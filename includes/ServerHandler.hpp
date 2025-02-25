#pragma once
#include "webserv.hpp"
#include "Request.hpp"

class HttpServer;

typedef struct s_client {
	int				fd;
	std::string		requestString;
	bool			requestReady;
	HttpRequest*	request;
	std::string		responseString;
	int				responseCode;
	bool			responseReady;
	std::time_t		lastRequest;
	bool			keep_alive;
} t_client;

class ServerHandler
{
private:
	std::vector<std::shared_ptr<HttpServer>>	_servers;
	size_t					 					_serverCount;
	std::vector<int>							_ports;
	std::unordered_map<int, t_client>			_clients;
	std::vector<struct pollfd>					_pollFds;
	bool										_running;
	ServerConfigData							_config;
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
	size_t		getPortCount();
	static void	signalHandler(int);

	void	printPollFds();

	// Helper functions for debugging
	// void	*get_in_addr(struct sockaddr *sa);
	// void	printServerData();
};
