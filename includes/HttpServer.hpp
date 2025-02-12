#pragma once

#include "webserv.hpp"
#include "ServerConfigData.hpp"

class HttpServer
{
private:
        Config  _settings; // stores the config for each server
        std::vector<struct addrinfo *>   _addr_info;
        std::vector<std::string>        _ports;
        std::vector<int>                _listen_sockfds;
        size_t                          _num_of_ports;
public:
        HttpServer(Config data);
        ~HttpServer();

        void            setupAddrinfo();

        // get functions
        const std::vector<struct addrinfo*> getAddrinfoVec();
        size_t                  getNumOfPorts();
        std::vector<int>        getListenSockfds();

        // set functions
        void    setNumOfPorts(size_t num);
        void    addSockfd(int listen_sockfd);
        void    setPorts(std::vector<std::string> ports);
};