#include "webserv.hpp"

void    setup_server()
{

}

void    run_server(struct addrinfo *serv)
{
	int sockfd; // socket descriptor

	if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
		perror("Socket");
		exit(errno);
	}
	if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
		perror("Bind");
		exit(errno);
	}
}