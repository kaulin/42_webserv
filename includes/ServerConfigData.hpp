#pragma once

#include "webserv.hpp"

class ServerConfigData {
private:
        std::string              _path;
	/*     struct addrinfo          _data;
        struct addrinfo          *_p;
        struct addrinfo          *_server_info;
        // std::vector<ServerConfigData>   _servers;
        std::string		        _host;
        uint16_t		        _port;
        std::string		        _server_name;
        std::string		        _error_page;
        size_t			        _client_body_size;
        std::vector<std::string>_routes; */
public:
    ServerConfigData();
    ~ServerConfigData();

    // class member functions
/*     void	setHost(std::string host);
    void	setPort(uint16_t port);
    void	setServerName(std::string server_name);
    void	setErrorPage(std::string error_page);
    void	setClientBodySize(size_t client_body_size);
    void	addRoute(std::string route); */
};