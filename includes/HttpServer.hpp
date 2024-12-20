#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer
{
private:
        std::vector<struct addrinfo*>   _addr_info;
        std::vector<std::string>        _ports;
        std::string     _name;
        int             _listen_sockfd;
        bool            _running;
        size_t          _num_of_ports;
public:
        HttpServer(class ServerConfigData server);
        ~HttpServer();

        void            setupAddrinfo();

        // get functions
        struct addrinfo *getAddrinfo();
        std::string     getName();
        int             getListenSockfd();
        int             getNumOfPorts();
        bool            isRunning();
        // set functions
        void            setNumOfPorts(size_t num);
        void            setSockfd(int listen_sockfd);
        void            setPorts(std::vector<std::string> ports);
};