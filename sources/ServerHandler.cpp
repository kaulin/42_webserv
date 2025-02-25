#include "webserv.hpp"
#include "ServerHandler.hpp"
#include "HttpServer.hpp"
#include <memory>
#include <csignal>

ServerHandler::ServerHandler(std::string path) : 
	_config(ServerConfigData(path))
{
	_server_count = _config.getServerCount();
	_servers.reserve(_server_count);
	_ports.reserve(_config.getPortCount());
	_pollfd_list.reserve(_config.getPortCount()); // reserves space for ports
	_running = false;
	std::cout << "Constructor Size of pollfd list: " << _pollfd_list.size() << "\n";
}

ServerHandler::~ServerHandler() 
{
	this->cleanupServers();
	_servers.clear();
	_pollfd_list.clear();

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

	/* Handles response */
void	ServerHandler::sendResponse(int client_sockfd)
{
	std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, world!";
   	ssize_t bytes_sent;
	std::cout << "Sending back response: " << "\n";
	if ((bytes_sent = send(client_sockfd, response.c_str(), response.length(), 0)) == -1) {
		error_and_exit("Send");
	}
	std::cout << "Response sent successfully!" 
	<< " (sent " << bytes_sent << " bytes)"
	<< std::endl;
}

/* 	One server config data instance is created. The config file is parsed in 
	the constructor and the appropriate values in the class instance is set. 
	Going through all serverBlock instances, a new shared pointer is made 
	for each virtual server with the appropriate configurations */
void    ServerHandler::setupServers()
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
}

void	ServerHandler::readRequest(int new_sockfd)
{
	int		numbytes;
	char	buf[1024];

	std::memset(buf, 0, 1024);
	if ((numbytes = recv(new_sockfd, buf, 1023, 0)) == -1) 
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
		int listen_sockfd = server->getListenSockfd();
		_pollfd_list[i].fd = listen_sockfd;
		_pollfd_list[i].events = POLLIN;
		i++;
	}
	printPollFds(); // for testing
}

void    ServerHandler::pollLoop()
{
	int 			event_count;
	int 			conn_sockfd;
	socklen_t		addrlen;
	struct sockaddr_storage remoteaddr_in;
	
	setPollList();
	while (_running)
	{
		if ((event_count = poll(_pollfd_list.data(), _pollfd_list.size(), -1)) == -1) {
			error_and_exit("Poll");
		}
		for(size_t i = 0; i < _pollfd_list.size(); i++)
		{
			if (_pollfd_list[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in);
				conn_sockfd = accept(_pollfd_list[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
				if (conn_sockfd == -1) 
				{
					error_and_exit("Accept"); // log error
				}
				readRequest(conn_sockfd);
				sendResponse(conn_sockfd);
				close(conn_sockfd);
			}
		}
	}
}

void	ServerHandler::setupSockets()
{
	for (auto& server : _servers)
	{
		struct addrinfo* ai = server->getAddrinfo();
		int	sockfd;
		int yes = 1;

		if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
			error_and_exit("Socket");
		}
		// sets the socket to non-blocking
		if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
			close(sockfd);
			error_and_exit("fcntl");
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			error_and_exit("Setsockopt");
		}
		if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) {
			close(sockfd);
			perror("Bind");
		}	
		if (ai == NULL) {
			error_and_exit("Failed to bind");
		}
		if (listen(sockfd, BACKLOG) == -1) {
			error_and_exit("Listen");
		}
		freeaddrinfo(ai);
		server->setSocket(sockfd);
	}
}

void	ServerHandler::signalHandler(int signal) 
{
	// handle shutdown
	std::cout << " Ctrl + C signal received, shutting down\n";
	exit(signal);
}

void    ServerHandler::runServers()
{
	std::signal(SIGINT, ServerHandler::signalHandler);
	std::signal(SIGPIPE, SIG_IGN);

	_running = true;
	setupSockets();
	pollLoop();
	cleanupServers();
}