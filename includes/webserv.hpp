#ifndef WEBSERV_HPP
#define WEBSERV_HPP

# include <iostream>
# include <cstring>
# include <sys/socket.h>
# include <sys/types.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <poll.h>
# include <sys/epoll.h>
# include <sys/wait.h>
# include <errno.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>

void    parse_config_file(const std::string conf, struct addrinfo serv_addr);

#endif