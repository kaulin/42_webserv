#include "ServerHandler.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

ServerHandler::ServerHandler() {
	_servers.clear();
	_pollfd_list.clear();
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
	std::cout << "Response sent successfully!" << " (sent " << bytes_sent << " bytes)"<< std::endl;
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

void	ServerHandler::handle_request(int new_sockfd)
{
	int		numbytes;
	char	buf[1024];

	std::memset(buf, 0, 1024);
	if ((numbytes = recv(new_sockfd, buf, 1023, 0)) == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cout << "No data available to read, try again later." << std::endl;
		} else {
			perror("recv");
			error_and_exit("Receive");
		}
	}
	std::cout << "Received request: " << buf << "\n";
	send_response(new_sockfd);
}

void	ServerHandler::setPollList()
{
	size_t	num_of_ports = 0;
	size_t	i = 0;

	for (auto& server : _servers) {
		num_of_ports += server.getNumOfPorts();
	}
	_pollfd_list.resize(num_of_ports);
	for (auto& server : _servers) // for each servers sockets
	{		
		std::vector<int> listen_sockfds = server.getListenSockfds();
		for (size_t j = 0; j < listen_sockfds.size() ; j++) {
			_pollfd_list[i].fd = listen_sockfds[j];
			_pollfd_list[i].events = POLLIN;
			i++;
		}
	}
	for (auto& poll_obj : _pollfd_list) {
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
}

void    ServerHandler::poll_loop()
{
	int 			poll_count;
	socklen_t		addrlen;
	int 			conn_sockfd;
	char			remoteIP[INET_ADDRSTRLEN];
	struct sockaddr_storage remoteaddr_in;
	
	setPollList();
	while (1) // while running
	{
		if ((poll_count = poll(_pollfd_list.data(), _pollfd_list.size(), -1)) == -1) {
			error_and_exit("Poll");
		}
		std::cout << "Listening to " << poll_count << " fd:s\n";
		for(size_t i = 0; i < _pollfd_list.size(); i++) 
		{
			if (_pollfd_list[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in);
				conn_sockfd = accept(_pollfd_list[i].fd, (struct sockaddr *)&remoteaddr_in, &addrlen);
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
	}
}

void    ServerHandler::runServers()
{
	int yes = 1;

	printServerData();
    for (auto& server : _servers)
    {
		std::vector<struct addrinfo*> server_addresses = server.getAddrinfoVec();
        int	sockfd;

        for (auto& curr_addr : server_addresses)
		{
	    	struct addrinfo *p = curr_addr;
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
				server.addSockfd(sockfd);
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
	poll_loop();
}