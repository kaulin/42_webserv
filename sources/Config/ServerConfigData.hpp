#pragma once
#include "webserv.hpp"
#include <map>
#include <vector>
#include <string>

struct Location {
    std::string _path;
    std::string _root;
    std::string _index;
    std::string _cgi_path;
    std::string _cgi_extension;
    bool        _dir_listing;
    std::pair<int, std::string> _redirect;
    std::unordered_map<std::string, bool> _methods = {{"GET", true}, {"POST", true}, {"DELETE", true}};
};

struct Config {
        std::string                             _host;
        std::string                             _name;
        std::vector<std::string>                _ports;
        size_t                                  _num_of_ports;
        size_t                                  _cli_max_bodysize;
        std::map<int, std::string>              _default_pages;
        std::map<int, std::string>              _error_pages;
        std::map<int, std::string>              _error_codes;
        std::vector<Location>                   _location;
        std::map<std::string, std::string>      _cgi_extensions;
};


class ServerConfigData {
private:
public:
        ServerConfigData(std::string path);
        ServerConfigData();
        ~ServerConfigData();

        std::map<std::string, Config>   serverConfigBlocks;

        // class member functions
        // void	setHost(const std::string& host);
        // void	setHost(const std::string&& host);
        // void	setServerName(const std::string& server_name);
        // void	setServerName(const std::string&& server_name);
        // void	setPorts(std::vector<std::string> ports);
        // void	setErrorPage(const std::string& error_page);
        // void	setErrorPage(const std::string&& error_page);
        // void	setClientBodySize(size_t client_body_size);
        // void	addRoute(std::string route);

        // const std::string&              getHost() const;
        // const std::vector<std::string>& getRoutes() const;
        // const std::vector<std::string>& getPorts() const;
        // const std::string&              getName() const;
        // size_t                          getNumOfPorts() const;
        // const std::string&              getErrorPage() const;
        // size_t                          getCliMaxBodysize() const;
        // void    printPorts() const;
};