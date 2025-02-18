#pragma once
#include "webserv.hpp"

class HttpServer
{
private:
        Config  _settings; // holds the config for each server
        std::vector<struct addrinfo *>   _addr_info;
        std::vector<std::string>        _ports;
        std::vector<int>                _listen_sockfds;
        size_t                          _num_of_ports;
public:
        HttpServer(Config data);
        ~HttpServer();

        void            setupAddrinfo();

        // get methods
        const std::vector<struct addrinfo*> getAddrinfoVec();
        size_t                  getNumOfPorts();
        std::vector<int>        getListenSockfds();

        // class methods
        void    addSockfd(int listen_sockfd);
};