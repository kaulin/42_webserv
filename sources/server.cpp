#include "webserv.hpp"
#define BACKLOG 10 // how many pending connections queue will hold

void	error_and_exit(const char *msg)
{
	perror(msg);
	exit(errno);
}

void	sigchild_handler(int s)
{
	int saved_status = errno;

	while (waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_status;
}


// small test handle request function, it simply reads the request and sends a "Hello, world!" response
void	handle_request(int new_sockfd)
{
	int		numbytes;
	char	buf[1024];

	std::memset(buf, 0, 1024); // make sure buffer is empty and null terminated
	if ((numbytes = recv(new_sockfd, buf, 1023, 0)) == -1) {
		error_and_exit("Receive");
	}
	std::cout << "Received request: " << buf << "\n";
	std::cout << "Sending back simple message.\n";
	if (send(new_sockfd, "Hello, world!", 13, 0) == -1) {
		error_and_exit("Send");
	}
}
void	setup_epoll(int listen_sockfd, int conn_sockfd)
{
	struct epoll_event ev, events[10];
	int nfds, epollfd;

	if ((epollfd = epoll_create1(0)) == -1) {
		error_and_exit("Epoll create");
	}
	ev.events = EPOLLIN; // read events
	ev.data.fd = listen_sockfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sockfd, &ev) == -1) {
		error_and_exit("Epoll control: listen_sockfd");
	}
}

void	accept_loop(int sockfd, struct sockaddr_storage addr_in)
{
	socklen_t	addr_size;
	int			new_sockfd; // socket for new incoming connections
	char 		ipstr_in[INET6_ADDRSTRLEN];

	// handle signals for child processes
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		error_and_exit("Sigaction");
	}
	// accept loop
	while (1) {
		addr_size = sizeof(struct sockaddr_storage);
		if ((new_sockfd = accept(sockfd, (struct sockaddr *)&addr_in, &addr_size)) == -1) {
			error_and_exit("Accept");
		}
		inet_ntop(addr_in.ss_family, get_in_addr((struct sockaddr *)&addr_in), ipstr_in, sizeof ipstr_in);
		std::cout << "Connected to: " << ipstr_in << "\n";
		if (fork() == 0) { // child process
			close(sockfd); // child doesn't need the listener
			handle_request(new_sockfd); // handle the request
			close(new_sockfd);
			exit(0); // exit child process with success
		}
		close(new_sockfd); // parent doesn't need client socket
	}
}

void    run_server(struct addrinfo *serv)
{
	int			sockfd; // socket file descriptors
	void		*buff;
	int			yes = 1;
	struct addrinfo 		*p;
	struct sockaddr_storage addr_in; // information about incoming connection goes here (who is calling from where)
	struct sigaction		sa;

	for (p = serv; p != NULL; p = p->ai_next)
	{
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
	freeaddrinfo(&serv_addr); // free the pointers alloc'd by getaddrinfo
	if (p == NULL) {
		error_and_exit("Failed to bind");
	}
	if (listen(sockfd, BACKLOG) == -1) {
		error_and_exit("Listen");
	}
	std::cout << "Server waiting for connections...: \n";
	setup_epoll(sockfd);
	accept_loop(sockfd, addr_in);
}