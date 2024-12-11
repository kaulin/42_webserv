#pragma once

#include "webserv.hpp"

class ServerConfig
{
private:
        std::string     _path;
	    struct addrinfo	_data;
        struct addrinfo *_p;
        struct addrinfo *_server_info;
        int	status;
public:
        ServerConfig();
        ~ServerConfig();

        // class member functions
        void    setConfigFilePath(std::string path);
        void    parseConfigFile(struct addrinfo serv_addr);
};