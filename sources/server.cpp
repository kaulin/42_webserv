#include "webserv.hpp"

void    setup_server()
{

}

void    run_server(struct addrinfo *serv)
{
	int			sockfd;
	int			new_sockfd; // socket file descriptor
	void		*buff;
	socklen_t 	addr_size;
	struct sockaddr_storage addr_in; // information about incoming connection goes here (who is calling from where)

	if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
		perror("Socket");
		exit(errno);
	}
	if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
		perror("Bind");
		exit(errno);
	}
	if (listen(sockfd, 10) == -1) {
		perror("Listen");
		exit(errno);
	}
	addr_size = sizeof(struct sockaddr_storage);
	if ((new_sockfd = accept(sockfd, (struct sockaddr *)&addr_in, &addr_size)) == 1) {
		perror("Accept");
		exit(errno);
	}
	
	if (recv(new_sockfd, buff, 100, 0) == -1) {
		perror("Receive");
		exit(errno);
	}
}