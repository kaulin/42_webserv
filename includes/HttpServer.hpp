#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer
{
private:
        struct addrinfo *_server_info;
        std::string     _name;
        int             _listen_sockfd;
        bool            _running;
        int             _num_of_ports;
public:
        HttpServer(class ServerConfigData server);
        ~HttpServer();

        void    runServer();
        void    setupAddrinfo();
        void    poll_loop();
        void    error_and_exit(const char *msg);
        void    handle_request(int sockfd);
        void    send_response(int sockfd_out, std::string response);
        void    *get_in_addr(struct sockaddr *sa);
};