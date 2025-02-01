#pragma once

#include "webserv.hpp"
#include <map>
#include <vector>
#include <string>

struct Location {
    std::string path;
    std::string root;
    std::string index;
    std::string cgi_path;
    std::string cgi_extension;
    bool        dir_listing;
    std::map<std::string, bool> methods = {{"GET", true}, {"POST", true}, {"DELETE", true}};
};

struct Config {
        std::string                     _host;
        std::string                     _name;
        std::vector<std::string>        _ports;
        std::vector<std::string>        _routes;
        size_t                          _num_of_ports;
        size_t                          _cli_max_bodysize;
        std::map<int, std::string>      _default_pages;
        std::map<int, std::string>      _error_pages;
        std::map<int, std::string>      _error_codes;
        std::vector<Location>           _location;
        std::map<std::string, std::string> cgi_extensions;
};

class ServerConfigData {
private:
public:
        ServerConfigData();
        ServerConfigData(std::string path);
        ~ServerConfigData();

        // class member functions
        std::map<std::string, std::vector<Config>> serverConfigs;
        
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