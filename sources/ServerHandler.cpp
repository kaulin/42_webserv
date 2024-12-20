#include "webserv.hpp"
#include "ServerHandler.hpp"
#include "ServerConfigData.hpp"

ServerHandler::ServerHandler() {
    _servers.clear();
    _server_count = 0;
}

ServerHandler::~ServerHandler() {}

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
	struct pollfd *poll_fds = new pollfd[_num_of_ports]();
	socklen_t		addrlen;
	int 			conn_sockfd;
	int 			poll_count;
	char			remoteIP[INET_ADDRSTRLEN];
	struct sockaddr_storage remoteaddr_in;

	std::cout << "Server waiting for connections...: \n";
	std::memset(poll_fds, 0, sizeof(struct pollfd) * _num_of_ports);
	if (poll_fds == NULL) {
		error_and_exit("Memset");
	}
	for (int i = 0; i < _num_of_ports; i++) {
		poll_fds[i].fd = _listen_sockfd;
		poll_fds[i].events = POLLIN;
	}
	while (1) // while running
	{
		std::cout << "Starting poll\n";
		if ((poll_count = poll(poll_fds, _num_of_ports, -1)) == -1) {
			error_and_exit("Poll");
		}
		std::cout << "Listening to " << poll_count << " fd:s\n";
		for(int i = 0; i < _num_of_ports; i++) 
		{
			if (poll_fds[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in);
				conn_sockfd = accept(_listen_sockfd, (struct sockaddr *)&remoteaddr_in, &addrlen);
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

void    ServerHandler::setupServers(std::vector<ServerConfigData> serverConfigs)
{
    for (const auto& config : serverConfigs) {
        HttpServer  serverInstance(config);
        serverInstance.setupAddrinfo();
		serverInstance.setPorts(config.ports);
        serverInstance.setNumOfPorts(config.num_of_ports);
        _servers.push_back(serverInstance);
    }
}

void    ServerHandler::runServers()
{
	int yes = 1;

    for (auto& server : _servers) // lop through each server
    {
        int     listen_sockfd;
	    struct addrinfo *p = server.getAddrinfo();
        
        for (p = server.getAddrinfo(); p != NULL; p = p->ai_next) {
            if ((listen_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                throw std::runtime_error("Socket");
                continue;
            }
            fcntl(listen_sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
            std::cout << "Listening to " << server.getListenSockfd() << " \n";
            if (setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                error_and_exit("Setsockopt");
            }
            if (bind(listen_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(listen_sockfd);
                perror("Bind");
                continue;
            }
            server.setSockfd(listen_sockfd);
            break;
        }
        if (p == NULL) {
            error_and_exit("Failed to bind");
        }
        if (listen(_listen_sockfd, BACKLOG) == -1) {
            error_and_exit("Listen");
        }
        freeaddrinfo(_server_info);
    }
	poll_loop();
}