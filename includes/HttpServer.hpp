#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer
{
private:
        std::string     _host;
        uint16_t        _port;
        int             _sockfd;
        bool            _running;
        struct pollfd   *_pollfds;
        int             _num_of_ports;
        struct addrinfo *_servinfo;
public:
        HttpServer(std::vector<ServerConfigData> _servers);
        ~HttpServer();

        // class member functions
        void    runServer();
        void    accept_loop(int sockfd, int epoll_fd, struct sockaddr_storage addr_in);
        void    poll_loop(int listen_sockfd);
        void    error_and_exit(const char *msg);
        void    sigchild_handler(int s);
        void    handle_request(int sockfd);
        void    send_response(int sockfd_out, std::string response);
};