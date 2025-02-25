#pragma once
#include "webserv.hpp"

class HttpServer
{
private:
        Config                          _settings; // holds the config for each server
        struct addrinfo*                _addr_info;
        std::string                     _port;
        int                             _listen_sockfd;
        size_t                          _num_of_ports;
public:
        HttpServer(Config data);
        ~HttpServer();

        void            setupAddrinfo();

        // get methods
        struct addrinfo*        getAddrinfo();
        size_t                  getNumOfPorts();
        int                     getListenSockfd();

        void                    setSocket(int sockfd);
};