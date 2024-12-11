#pragma once

#include "webserv.hpp"

class HttpServer
{
private:
        std::string     _host;
        uint16_t        _port;
        int             _sockfd;
        bool            _running;
        struct pollfd   *_pollfds;
        struct addrinfo _addrinfo;
public:
        HttpServer();
        ~HttpServer();

        // class member functions
        void    runServer(struct addrinfo *serv);
};