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

	std::memset(buf, 0, 1024); // make sure buffer is empty and null terminated
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
	char			remoteIP[INET6_ADDRSTRLEN];
	struct sockaddr_storage remoteaddr_in; // address of incoming connection

	std::cout << "Server waiting for connections...: \n";
	std::memset(poll_fds, 0, sizeof(struct pollfd) * _num_of_ports);
	std::cout << "1...: \n";
	if (poll_fds == NULL) {
		error_and_exit("Memset");
	}
	// loop to poll for each port that is listened to
	for (int i = 0; i < _num_of_ports; i++) {
		poll_fds[i].fd = _listen_sockfd;
		poll_fds[i].events = POLLIN; // what type of events we are looking for (incoming data)
	}
	std::cout << "2...: \n";
	while (_running) {
		if ((poll_count = poll(poll_fds, _num_of_ports, -1)) == -1) {
			error_and_exit("Poll");
		}
		for(int i = 0; i < _num_of_ports; i++) 
		{
			std::cout << "3...: " << i << " \n";
			if (poll_fds[i].revents & POLLIN) 
			{
				addrlen = sizeof(remoteaddr_in); // save the size of the address
				conn_sockfd = accept(_listen_sockfd, (struct sockaddr *)&remoteaddr_in, &addrlen);
				if (conn_sockfd == -1) {
					error_and_exit("Accept");
				}
				printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr_in.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr_in),
                                remoteIP, INET6_ADDRSTRLEN), conn_sockfd);
				handle_request(conn_sockfd);
				close(conn_sockfd);
			}
		}
	}
}

void    HttpServer::runServer()
{
	int						sockfd; // socket file descriptors
	int						yes = 1;
	struct addrinfo 		*p = _server_info;

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
	struct addrinfo	data;

	// should loop through each server config and create a new Server Data object
	std::memset(&data, 0, sizeof data); // set data to be empty
	data.ai_family = AF_UNSPEC; // IPv4 or IPv6
	data.ai_socktype = SOCK_STREAM; // TCP stream socket
	data.ai_flags = AI_PASSIVE; // auto fills IP address - sets to localhost's IP
	if ((status = getaddrinfo(NULL, "3490", &data, &_server_info)) != 0) {
		std::cerr << gai_strerror(EXIT_FAILURE);
		exit(1);
	}
	// servinfo now points to a linked list of 1 or more struct addrinfos
}
