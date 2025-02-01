#include "ServerHandler.hpp"
#include "ConfigParser.hpp"
#include "ServerConfigData.hpp"
#include "HttpServer.hpp"
#include "webserv.hpp"
#include <memory>

#define BACKLOG 10 // how many pending connections queue will hold

ServerHandler::ServerHandler() {
	_servers.clear();
	_pollfd_list.clear();
	_ports.clear();
	_server_count = 0;
	_running = true;
}

ServerHandler::~ServerHandler() 
{
	_servers.clear();
	_pollfd_list.clear();
}

void	ServerHandler::error_and_exit(const char *msg)
{
	std::string errmsg = "Webserver: " + std::string(msg);
	perror(errmsg.c_str());
	exit(errno);
}

void	ServerHandler::cleanupServers()
{
	for (auto& server : _servers) {
		std::vector<int> listen_sockfds = server->getListenSockfds();
		for (auto& sockfd : listen_sockfds) {
			close(sockfd);
		}
	}
}

void	ServerHandler::send_response(int client_sockfd)
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

void    ServerHandler::setupServers(std::string path)
{
	ServerConfigData config(path);
	// makes a new shared pointer for each virtual server

	for (const auto& [servName, config] : config.serverConfigs) {
		std::shared_ptr<HttpServer> virtual_server = std::make_shared<HttpServer>(config); // sets up the server with the current config
		_servers.push_back(virtual_server);
	}

    /* for (const auto& current : configs) {
        HttpServer  serverInstance(current);
		serverInstance.setName(current.getName());
		serverInstance.setPorts(current.getPorts());
        serverInstance.setNumOfPorts(current.getNumOfPorts());
        serverInstance.setupAddrinfo();
        _servers.push_back(serverInstance);
    } */
}

void	ServerHandler::read_request(int new_sockfd)
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

	_pollfd_list.resize(num_of_ports);
	for (auto& server : _servers)
	{		
		std::vector<int> listen_sockfds = server->getListenSockfds();
		for (size_t j = 0; j < listen_sockfds.size() ; j++) {
			_pollfd_list[i].fd = listen_sockfds[j];
			_pollfd_list[i].events = POLLIN;
			i++;
		}
	}
	/* 	for (auto& poll_obj : _pollfd_list) {
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	} */
}

void    ServerHandler::poll_loop()
{
	int 			poll_count;
	int 			conn_sockfd;
	socklen_t		addrlen;
	struct sockaddr_storage remoteaddr_in;
	
	setPollList();
	while (_running)
	{
		if ((poll_count = poll(_pollfd_list.data(), _pollfd_list.size(), -1)) == -1) {
			error_and_exit("Poll");
		}
		for(size_t i = 0; i < _pollfd_list.size(); i++)
		{
			if (_pollfd_list[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in);
				conn_sockfd = accept(_pollfd_list[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
				if (conn_sockfd == -1) {
					error_and_exit("Accept");
				}
				read_request(conn_sockfd);
				send_response(conn_sockfd);
				close(conn_sockfd);
			}
		}
	}
}

void	ServerHandler::setupSockets()
{
	for (auto& server : _servers)
	{
		std::vector<struct addrinfo*> server_addresses = server->getAddrinfoVec();
		int	sockfd;

		for (auto& curr_addr : server_addresses)
		{
			struct addrinfo *p = curr_addr;
			int yes = 1;
			for (p = curr_addr; p != NULL; p = p->ai_next) 
			{
				if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
					error_and_exit("Socket");
					continue;
				}
				fcntl(sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
					error_and_exit("Setsockopt");
				}
				if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(sockfd);
					perror("Bind");
					continue;
				}
				server->addSockfd(sockfd);
				break;
			}
			if (p == NULL) {
				error_and_exit("Failed to bind");
			}
			if (listen(sockfd, BACKLOG) == -1) {
				error_and_exit("Listen");
			}
			freeaddrinfo(curr_addr);
		}
	}
}

void    ServerHandler::runServers()
{
	// printServerData();
	_running = true;
	setupSockets();
	poll_loop();
	cleanupServers();
}