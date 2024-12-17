#pragma once

#include "webserv.hpp"

class ServerConfigData {
public:
        std::string     host;
        uint16_t        port;
        std::string     name;
        std::string     error_page;
        size_t          cli_max_bodysize;
        std::vector<std::string>    routes;

        ServerConfigData();
        ~ServerConfigData();

    // class member functions
/*  void	setHost(std::string host);
    void	setPort(uint16_t port);
    void	setServerName(std::string server_name);
    void	setErrorPage(std::string error_page);
    void	setClientBodySize(size_t client_body_size);
    void	addRoute(std::string route); */
};