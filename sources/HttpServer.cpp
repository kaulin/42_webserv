#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigData.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(ServerConfigData server)
{
	_server_info = nullptr;
	_listen_sockfd = -1;
	_running = true;
	_num_of_ports = 1;
	_name = server.name;
}

HttpServer::~HttpServer() {}

void	HttpServer::error_and_exit(const char *msg)
{
	perror(msg);
	exit(errno);
}

void	*HttpServer::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void	HttpServer::send_response(int sockfd_out, std::string response)
{
	std::cout << "Sending back response: " << response << "\n";
	if (send(sockfd_out, response.c_str(), response.length(), 0) == -1) {
		error_and_exit("Send");
	}
}

// small test handle request function, it simply reads the request and sends a "Hello, world!" response
void	HttpServer::handle_request(int new_sockfd)
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

void    HttpServer::poll_loop()
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

void    HttpServer::runServer()
{
	int						yes = 1;
	struct addrinfo 		*p = _server_info;

	for (p = _server_info; p != NULL; p = p->ai_next) {
		if ((_listen_sockfd = socket(_server_info->ai_family, _server_info->ai_socktype, _server_info->ai_protocol)) == -1) {
			perror("Socket");
			continue;
		}
		std::cout << "Listening to " << _listen_sockfd << " \n";
		fcntl(_listen_sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
		if (setsockopt(_listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			error_and_exit("Setsockopt");
		}
		if (bind(_listen_sockfd, _server_info->ai_addr, _server_info->ai_addrlen) == -1) {
			close(_listen_sockfd);
			perror("Bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		error_and_exit("Failed to bind");
	}
	if (listen(_listen_sockfd, BACKLOG) == -1) {
		error_and_exit("Listen");
	}
	freeaddrinfo(_server_info);
	poll_loop();
}

void	HttpServer::setupAddrinfo()
{
	int	status;
	struct addrinfo	hints;

	std::memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket

	if ((status = getaddrinfo("localhost", "8080", &hints, &_server_info)) != 0) {
		throw std::runtime_error(gai_strerror(status));
	}
	std::cout << "Getaddrinfo finished with status: " << status << "\n";
	
	// test function for printing server host addresses
	struct addrinfo *p;
	for(p = _server_info; p != NULL; p = p->ai_next) {
        void *addr;
		char ipstr[INET_ADDRSTRLEN];

        struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv->sin_addr);
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("Address: %s\n", ipstr);
    }

}
