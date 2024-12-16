#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigData.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

HttpServer::HttpServer(std::vector<ServerConfigData> servers)
{
	_host = "localhost";
	_port = 80;
	sockfd = 0;
	_running = true;
	_pollfds = NULL;
	_num_of_ports = servers.size();
	_servinfo = NULL;
}

HttpServer::~HttpServer() {}

void	HttpServer::error_and_exit(const char *msg)
{
	perror(msg);
	exit(errno);
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
	this.send_response(new_sockfd, "Hello, world!");
}
/*
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
struct pollfd {
               int   fd;         file descriptor
               short events;     requested events
               short revents;    returned events
           };
*/
void    HttpServer::poll_loop(int listen_sockfd)
{
	struct epoll_event	ev, events[10];
	int conn_sockfd;
	int poll_count;

	_pollfds = std::memset(listen_pollfds, 0, sizeof(struct pollfd) * _num_of_ports);
	if (_pollfds == NULL) {
		error_and_exit("Memset");
	}
	// loop to poll for each port that is listened to
	for (int i = 0; i < _num_of_ports; i++) {
		_pollfds[i].fd = listen_sockfd;
		_pollfds[i].events = POLLIN; // what type of events we are looking for (incoming data)
	}
	while (true) {
		if ((poll_count = poll(_pollfds, _num_of_ports, -1)) == -1) {
			error_and_exit("Poll");
		}
		for(int i = 0; i < _num_of_ports; i++) 
		{
			if (_pollfds[i].revents & POLLIN) 
			{
				conn_sockfd = accept(listen_sockfd, NULL, NULL);
				if (conn_sockfd == -1) {
					error_and_exit("Accept");
				}
				/* this.handle_request(conn_sockfd);
				close(conn_sockfd);
				break; */
			}
		}
	}
}

void    HttpServer::accept_loop(int sockfd, int epoll_fd, struct sockaddr_storage addr_in)
{
	socklen_t	addr_size;
	int			new_sockfd; // socket for new incoming connections
	int 		n_pollfds;
	char 		ipstr_in[INET6_ADDRSTRLEN];
	struct sigaction		sa;

	// handle signals for child processes
/* 	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; */
/* 	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		error_and_exit("Sigaction");
	} */
	// accept loop, loop until running = false
	while (1) {
		if ((n_pollfds = epoll_wait(epoll_fd, events, 10, 0)) == -1) {
			error_and_exit("Epoll wait");
		}
		addr_size = sizeof(struct sockaddr_storage);
		if ((new_sockfd = accept(sockfd, (struct sockaddr *)&addr_in, &addr_size)) == -1) {
			error_and_exit("Accept");
		}
		inet_ntop(addr_in.ss_family, get_in_addr((struct sockaddr *)&addr_in), ipstr_in, sizeof ipstr_in);
		std::cout << "Server: got connection from " << ipstr_in << "\n";
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
	struct addrinfo 		*p = _servinfo;
	struct sockaddr_storage addr_in; // information about incoming connection goes here (who is calling from where)

	for (p = serv; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
			perror("Socket");
			continue;
		}
		fcntl(sockfd, F_SETFL, O_NONBLOCK); // sets the socket to non-blocking
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			error_and_exit("Setsockopt");
		}
		if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
			close(sockfd);
			perror("Bind");
			continue;
		}
		break;
	}
	freeaddrinfo(serv); // free the pointers alloc'd by getaddrinfo
	if (p == NULL) {
		error_and_exit("Failed to bind");
	}
	if (listen(sockfd, BACKLOG) == -1) {
		error_and_exit("Listen");
	}
	std::cout << "Server waiting for connections...: \n";
	// this.poll_loop(sockfd);
	// this.accept_loop(sockfd, epoll_fd, addr_in);
}