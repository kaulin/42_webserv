#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer;

class ServerConfigParser { 
private:
        std::string _path;
public:
        ServerConfigParser();
        ~ServerConfigParser();

        std::vector<ServerConfigData> _servers;      

        // class member functions
        void    parseConfigFile();
        void    setConfigFilePath(std::string path);
};