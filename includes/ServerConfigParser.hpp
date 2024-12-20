#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer;

class ServerConfigParser { 
private:
        std::string     _path;
        static size_t   _num_of_servers;
public:
        ServerConfigParser();
        ~ServerConfigParser();

        std::vector<ServerConfigData> servers;      

        // class member functions
        void    parseConfigFile();
        void    setConfigFilePath(std::string path);
};