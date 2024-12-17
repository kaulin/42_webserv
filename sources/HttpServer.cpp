#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigData.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(ServerConfigData server)
{
	// data should come from server info file
	_server_info = nullptr;
	_listen_sockfd = -1;
	_running = true;
	_num_of_ports = 1;
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

void	HttpServer::sigchild_handler(int s)
{
	int saved_status = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_status;
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

	std::memset(buf, 0, 1024); // make sure buffer is empty and null terminated
	if ((numbytes = recv(new_sockfd, buf, 1023, 0)) == -1) {
		error_and_exit("Receive");
	}
	std::cout << "Received request: " << buf << "\n";
	send_response(new_sockfd, "Hello, world!");
}

void    HttpServer::poll_loop()
{
	struct pollfd	*poll_fds;
	socklen_t		addrlen;
	int 			conn_sockfd;
	int 			poll_count;
	struct sockaddr_storage remoteaddr; // address of incoming connection
	char			remoteIP[INET6_ADDRSTRLEN];

	std::cout << "Server waiting for connections...: \n";
	std::memset(poll_fds, 0, sizeof(struct pollfd) * _num_of_ports);
	if (poll_fds == NULL) {
		error_and_exit("Memset");
	}
	// loop to poll for each port that is listened to
	for (int i = 0; i < _num_of_ports; i++) {
		poll_fds[i].fd = _listen_sockfd;
		poll_fds[i].events = POLLIN; // what type of events we are looking for (incoming data)
	}
	while (_running) {
		if ((poll_count = poll(poll_fds, _num_of_ports, -1)) == -1) {
			error_and_exit("Poll");
		}
		for(int i = 0; i < _num_of_ports; i++) 
		{
			if (poll_fds[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr); // save the size of the address
				conn_sockfd = accept(_listen_sockfd, (struct sockaddr *)&remoteaddr, &addrlen);
				if (conn_sockfd == -1) {
					error_and_exit("Accept");
				}
				printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN), conn_sockfd);
				handle_request(conn_sockfd);
				close(conn_sockfd);
			}
		}
	}
}

void    HttpServer::accept_loop(int sockfd, int epoll_fd, struct sockaddr_storage addr_in)
{
	socklen_t			addr_size;
	int					new_sockfd; // socket for new incoming connections
	struct sigaction	sa;

	// handle signals for child processes
/* 	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; */
/* 	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		error_and_exit("Sigaction");
	} */
	// accept loop, loop until running = false
	while (1) {
		addr_size = sizeof(struct sockaddr_storage);
		if ((new_sockfd = accept(sockfd, (struct sockaddr *)&addr_in, &addr_size)) == -1) {
			error_and_exit("Accept");
		}
		if (fork() == 0) { // child process
			close(sockfd); // child doesn't need the listener
			handle_request(new_sockfd); // handle the request
			close(new_sockfd);
			exit(0); // exit child process with success
		}
		close(new_sockfd); // parent doesn't need client socket
	}
}

void    HttpServer::runServer()
{
	int						sockfd; // socket file descriptors
	void					*buff;
	int						yes = 1;
	struct addrinfo 		*p = _server_info;
	struct sockaddr_storage addr_in; // information about incoming connection goes here (who is calling from where)

	for (p = _server_info; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(_server_info->ai_family, _server_info->ai_socktype, _server_info->ai_protocol)) == -1) {
			perror("Socket");
			continue;
		}
		fcntl(sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			error_and_exit("Setsockopt");
		}
		if (bind(sockfd, _server_info->ai_addr, _server_info->ai_addrlen) == -1) {
			close(sockfd);
			perror("Bind");
			continue;
		}
		break;
	}
	freeaddrinfo(_server_info); // free the pointers alloc'd by getaddrinfo
	if (p == NULL) {
		error_and_exit("Failed to bind");
	}
	if (listen(sockfd, BACKLOG) == -1) {
		error_and_exit("Listen");
	}
	poll_loop();
}

void	HttpServer::setupAddrinfo()
{
	int	status;
	struct addrinfo	data; // we set to data some preliminary data that gets applied in gettaddrinfo

	// Some temp data to test out server info where we allow incoming connections
	// should loop through each server config and create a new Server Data object
	std::memset(&data, 0, sizeof data); // set data to be empty
	data.ai_family = AF_UNSPEC; // IPv4 or IPv6
	data.ai_socktype = SOCK_STREAM; // TCP stream socket
	data.ai_flags = AI_PASSIVE; // auto fills IP address - sets to localhost's IP

	if ((status = getaddrinfo(NULL, "80", &data, &_server_info)) != 0) {
		std::cerr << gai_strerror(EXIT_FAILURE);
		exit(1);
	}
	// servinfo now points to a linked list of 1 or more struct addrinfos
}
