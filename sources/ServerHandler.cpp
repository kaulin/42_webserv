#include "ServerHandler.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

ServerHandler::ServerHandler() {
	_servers.clear();
	_server_count = 0;
}

ServerHandler::~ServerHandler() {}

void	ServerHandler::printServerData()
{
	std::vector<int> listensockfds;
	// for testing
	for (auto& server : _servers) {
        std::cout << "Host: " << server.getName() << "\n";
		listensockfds = server.getListenSockfds();
		for (auto& curr : listensockfds) {
			std::cout << "Listen sockfd: " << curr << "\n";
		}
        std::cout << "Number of ports: " << server.getNumOfPorts()
        << "\n--------------------------\n";
    }
}

void	ServerHandler::error_and_exit(const char *msg)
{
	perror(msg);
	exit(errno);
}

void	*ServerHandler::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void	ServerHandler::send_response(int sockfd_out, std::string response)
{
	std::cout << "Sending back response: " << response << "\n";
	if (send(sockfd_out, response.c_str(), response.length(), 0) == -1) {
		error_and_exit("Send");
	}
}

// small test handle request function, it simply reads the request and sends a "Hello, world!" response
void	ServerHandler::handle_request(int new_sockfd)
{
	int		numbytes;
	char	buf[1024];

	std::memset(buf, 0, 1024);
	if ((numbytes = recv(new_sockfd, buf, 1023, 0)) == -1) {
		error_and_exit("Receive");
	}
	std::cout << "Received request: " << buf << "\n";
	send_response(new_sockfd, "Hello, world!");
}

void    ServerHandler::poll_loop()
{
	int 			poll_count;
	socklen_t		addrlen;
	int 			conn_sockfd;
	char			remoteIP[INET_ADDRSTRLEN];

	struct sockaddr_storage remoteaddr_in;
	// std::vector<pollfd_obj> poll_list;

	std::vector< struct pollfd> pollfd_list;
	pollfd_list.clear();

	// std::memset(&poll_list, 0, sizeof(struct pollfd) * _server_count);

	for (auto& server : _servers) // for each servers sockets
	{
		std::cout << "Server " << server.getName() << " waiting for connections...: \n";
		std::vector<int> listen_sockfds = server.getListenSockfds();
		// size_t num_of_ports = server.getNumOfPorts();
		pollfd_list.resize(server.getNumOfPorts());
		for (size_t i = 0; i < server.getNumOfPorts(); i++) {
			pollfd_list[i].fd = listen_sockfds[i];
			pollfd_list[i].events = POLLIN;
		}
	}
	for (auto& poll_obj : pollfd_list)
	{
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
	while (1) // while running
	{
		if ((poll_count = poll(pollfd_list.data(), pollfd_list.size(), -1)) == -1) {
			error_and_exit("Poll");
		}
		std::cout << "Listening to " << poll_count << " fd:s\n";
		for(size_t i = 0; i < pollfd_list.size(); i++) 
		{
			if (pollfd_list[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in);
				conn_sockfd = accept(pollfd_list[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
				if (conn_sockfd == -1) {
					error_and_exit("Accept");
				}
				printf("pollserver: new connection from %s on "
							"socket %d\n",
							inet_ntop(remoteaddr_in.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr_in),
								remoteIP, INET_ADDRSTRLEN), conn_sockfd);
				handle_request(conn_sockfd);
				close(conn_sockfd);
			}
		}
		/* for (auto& poll_obj : pollfd_list)
		{
			std::cout << "Starting poll\n";
			if ((poll_count = poll(poll_obj.poll_fds, poll_obj.num_of_ports, -1)) == -1) {
				error_and_exit("Poll");
			}
			std::cout << "Listening to " << poll_count << " fd:s\n";
			for(size_t i = 0; i < poll_obj.num_of_ports; i++) 
			{
				if (poll_obj.poll_fds[i].revents & POLLIN) 
				{
					addrlen = sizeof(remoteaddr_in);
					conn_sockfd = accept(poll_obj.poll_fds[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
					if (conn_sockfd == -1) {
						error_and_exit("Accept");
					}
					printf("pollserver: new connection from %s on "
								"socket %d\n",
								inet_ntop(remoteaddr_in.ss_family,
									get_in_addr((struct sockaddr*)&remoteaddr_in),
									remoteIP, INET_ADDRSTRLEN), conn_sockfd);
					handle_request(conn_sockfd);
					close(conn_sockfd);
				}
			}
		} */
	}
}

void    ServerHandler::setupServers(std::vector<ServerConfigData> configs)
{
    for (const auto& current : configs) {
        HttpServer  serverInstance(current);
		serverInstance.setPorts(current.getPorts());
        serverInstance.setNumOfPorts(current.getNumOfPorts());
        serverInstance.setupAddrinfo();
        _servers.push_back(serverInstance);
    }
}

void    ServerHandler::runServers()
{
	int yes = 1;

	// create a socket for each server, bind and listen
	printServerData(); // for testing
    for (auto& server : _servers)
    {
        int	sockfd;
		std::vector<struct addrinfo*> server_addresses = server.getAddrinfoVec();

		std::cout << "Setting up server " << server.getName() << "\n"; 
        for (auto& curr_addr : server_addresses) // loop through each address info
		{
	    	struct addrinfo *p = curr_addr; // pointer to current address info
			
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
					error_and_exit("Bind");
					continue;
				}
				server.setSockfd(sockfd);
				break;
			}
			if (p == NULL) {
				error_and_exit("Failed to bind");
			}
			std::vector<int> listenFds = server.getListenSockfds(); 
			for (auto& currFd : listenFds)
			{
				if (listen(currFd, BACKLOG) == -1) {
					error_and_exit("Listen");
				}
			}
			freeaddrinfo(curr_addr);
        }
		server.printListenFds();
    }
	poll_loop();
}