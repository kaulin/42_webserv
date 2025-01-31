#pragma once

#include "webserv.hpp"

class ServerConfigData;

class HttpServer;

class ConfigParser { 
private:
        ConfigParser() = delete;
        ConfigParser(const ConfigParser &other) = delete;
        ConfigParser& operator=(const ConfigParser &other) = delete;
public:
        // class member functions
        static std::map<std::string, std::vector<Config>> parseConfigFile(std::string path);
        static void     checkConfigFilePath(std::string path);
};