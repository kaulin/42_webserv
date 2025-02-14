#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include "webserv.hpp"
#include "ServerConfigData.hpp"
#include <algorithm>
#include <regex>
#include "../../includes/webserv.hpp" //testing with relative path

class ServerConfigData;

class HttpServer;

class ConfigParser { 
private:
        ConfigParser() = delete;
        ConfigParser(const ConfigParser &other) = delete;
        ConfigParser& operator=(const ConfigParser &other) = delete;
public:
        // class member functions
	static std::map<std::string, Config>	parseConfigFile(std::string path);
    	static void                             checkConfigFilePath(std::string path);
	static std::string                      read_file(std::string path);
	static std::vector<std::string>         tokenize(std::string &file_content);
        static std::unordered_map<std::string, std::vector<std::string>>	assignKeyToValue(std::vector<std::string> &tokens, std::vector<std::string>::iterator &it);

};
