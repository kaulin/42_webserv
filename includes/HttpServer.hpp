#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer
{
private:
        struct addrinfo *_server_info;
        struct pollfd   *_pollfds;
        int             _sockfd;
        bool            _running;
        int             _num_of_ports;
public:
        HttpServer(std::vector<ServerConfigData> _servers);
        ~HttpServer();

        // class member functions
        void    runServer();
        void    setupAddrinfo();
        void    accept_loop(int sockfd, int epoll_fd, struct sockaddr_storage addr_in);
        void    poll_loop(int listen_sockfd);
        void    error_and_exit(const char *msg);
        void    sigchild_handler(int s);
        void    handle_request(int sockfd);
        void    send_response(int sockfd_out, std::string response);
};