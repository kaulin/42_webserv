#pragma once
#include <ctime>
#include <memory>
#include <poll.h>
#include <deque>
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

enum PollType 
{
	SET_POLLBOTH,
	SET_POLLOUT,
	SET_POLLIN
};

class ServerHandler
{
private:
	static bool											_sigintReceived;
	size_t												_serverCount;
	std::vector<int>									_ports;
	std::vector<std::unique_ptr<HttpServer>>			_servers;
	std::unordered_map<int, std::unique_ptr<Client>>	_clients;
	std::unordered_map<int, Client*>					_resourceFds;
	std::vector<struct pollfd>							_pollFds;
	std::vector<struct pollfd>							_newPollFds;
	std::deque<int>										_fdsToDrop;
	ServerConfigData									_config;
	CGIHandler											_CGIHandler;
	static void	resetClient(Client& client);
	static void	signalHandler(int);
	void		readFromFd(size_t& i);
	void		writeToFd(size_t& i);
	void		addResourceFd(Client& client);
	void		removeResourceFd(int fd);
	void		addToPollList(int fd, PollType pollType);
	void		removeFromPollList(int fd);
	void		updatePollList();
	void		pollLoop();
	void		setPollList();
	void		addConnection(size_t& i);
	void		closeConnection(size_t& i);
	bool		checkTimeout(const Client& client);
	void		checkClients();
	void		handleServerException(int statusCode, size_t& fd);
public:
	ServerHandler(std::string path);
	~ServerHandler();

	void		runServers();
	void		setupServers();
	

	// Helper functions for debugging
	// void	*get_in_addr(struct sockaddr *sa);
	// void	printServerData();
};
