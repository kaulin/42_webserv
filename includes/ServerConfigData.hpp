#pragma once

#include "webserv.hpp"

class ServerConfigData {
private:
        std::string                 _host;
        std::vector<std::string>    _ports;
        std::vector<std::string>    _routes;
        size_t                      _num_of_ports;
        std::string                 _name;
        std::string                 _error_page;
        size_t                      _cli_max_bodysize;
public:
        ServerConfigData();
        ~ServerConfigData();

        // class member functions
        void	setHost(const std::string& host);
        void	setHost(const std::string&& host);
        void	setServerName(const std::string& server_name);
        void	setServerName(const std::string&& server_name);
        void	setPorts(std::vector<std::string> ports);
        void	setErrorPage(const std::string& error_page);
        void	setErrorPage(const std::string&& error_page);
        void	setClientBodySize(size_t client_body_size);
        void	addRoute(std::string route);

        const std::string&              getHost() const;
        const std::vector<std::string>& getRoutes() const;
        const std::vector<std::string>& getPorts() const;
        const std::string&              getName() const;
        size_t                          getNumOfPorts() const;
        const std::string&              getErrorPage() const;
        size_t                          getCliMaxBodysize() const;

        void    printPorts() const;
};