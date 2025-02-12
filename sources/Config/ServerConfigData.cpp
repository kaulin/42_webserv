#include "webserv.hpp"
#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"

ServerConfigData::ServerConfigData()
{
    // use default server
}

void    ServerConfigData::printConfigs()
{
    for (const auto &config : serverConfigBlocks)
    {
        std::cout << "Host: " << config.second._host << "\n"; 
        std::cout << "Names: ";
        for (const auto& name : config.second._names)
            std::cout << name << " ";
        std::cout << "\n";
        
        std::cout << "Ports: ";
        for (const auto& port : config.second._ports)
            std::cout << port << " ";
        std::cout << "\n";
        
        std::cout << "Number of Ports: " << config.second._num_of_ports << "\n";
        std::cout << "Client Max Body Size: " << config.second._cli_max_bodysize << "\n";
        
        std::cout << "Default Pages:\n";
        for (const auto& page : config.second._default_pages)
            std::cout << "  " << page.first << ": " << page.second << "\n";
        
        std::cout << "Error Pages:\n";
        for (const auto& page : config.second._error_pages)
            std::cout << "  " << page.first << ": " << page.second << "\n";
        
        std::cout << "Error Codes:\n";
        for (const auto& code : config.second._error_codes)
            std::cout << "  " << code.first << ": " << code.second << "\n";
        
        std::cout << "CGI Params:\n";
        for (const auto& param : config.second._cgi_params)
            std::cout << "  " << param.first << " = " << param.second << "\n";
        
        std::cout << "Locations:\n";
        for (const auto& loc : config.second._location) {
            const Location& location = loc.second;
            std::cout << "  Path: " << location._path << "\n";
            std::cout << "  Root: " << location._root << "\n";
            std::cout << "  Index: " << location._index << "\n";
            std::cout << "  CGI Path: " << location._cgi_path << "\n";
            std::cout << "  CGI Param: " << location._cgi_param << "\n";
            std::cout << "  Redirect: " << location._redirect.first << " -> " << location._redirect.second << "\n";
            std::cout << "  Directory Listing: " << (location._dir_listing ? "Enabled" : "Disabled") << "\n";
            
            std::cout << "  Methods:\n";
            for (const auto& method : location._methods)
                std::cout << "    " << method.first << ": " << (method.second ? "Allowed" : "Not Allowed") << "\n";
            
            std::cout << "\n";
        }
    }
}

ServerConfigData::ServerConfigData(std::string path) 
{
    // needs to set the server configs for each server
    // ServerConfigBlocks is of datastructure = std::map<std::string, std::vector<Config>>
    serverConfigBlocks = ConfigParser::parseConfigFile(path);
/*     
    _host.clear();
    _name.clear();
    _routes.clear();
    _ports.clear();
    _cli_max_bodysize = 0;
    _location.clear(); */

    printConfigs();
	std::cout << "New server config data created...: \n";
}

ServerConfigData::~ServerConfigData() {
    std::cout << "Server config data instance deleted\n";
}

std::map<std::string, Config>&	ServerConfigData::getConfigBlocks()
{
    return (this->serverConfigBlocks);
}