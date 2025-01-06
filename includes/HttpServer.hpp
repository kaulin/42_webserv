#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer
{
private:
        std::vector<struct addrinfo *>   _addr_info;
        std::vector<std::string>        _ports;
        std::string                     _name;
        std::vector<int>                _listen_sockfds;
        bool                            _running;
        size_t                          _num_of_ports;
public:
        HttpServer(class ServerConfigData server);
        ~HttpServer();

        void            setupAddrinfo();
        bool            isRunning();

        // get functions
        const std::vector<struct addrinfo*> getAddrinfoVec();
        std::string             getName();
        size_t                  getNumOfPorts();
        std::vector<int>        getListenSockfds();
        // set functions
        void            setNumOfPorts(size_t num);
        void            addSockfd(int listen_sockfd);
        void            setPorts(std::vector<std::string> ports);
};