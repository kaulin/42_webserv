#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"

ServerConfigData::ServerConfigData(std::string path) 
{
    // needs to set the server configs for each server
    serverConfigs = ConfigParser::parseConfigFile(path);
/*     
    _host.clear();
    _name.clear();
    _routes.clear();
    _ports.clear();
    _cli_max_bodysize = 0;
    _location.clear(); */

    // Print server data
	std::cout << "New server config data created...: \n";
}

ServerConfigData::~ServerConfigData() {}

/* void ServerConfigData::setHost(const std::string& host) 
{
    _host = host;
}

void ServerConfigData::setHost(const std::string&& host) 
{
    _host = std::move(host);
}

void ServerConfigData::setPorts(std::vector<std::string> ports)
{
    for (const auto& current : ports) {
        _ports.push_back(current);
    }
    _num_of_ports = _ports.size();
}

void ServerConfigData::setServerName(const std::string& server_name) 
{
    _name = server_name;
}

void ServerConfigData::setServerName(const std::string&& server_name) 
{
    _name = std::move(server_name);
}

void ServerConfigData::setErrorPage(const std::string& error_page) 
{
    _error_page = error_page;
}

void ServerConfigData::setErrorPage(const std::string&& error_page) 
{
    _error_page = std::move(error_page);
}

void ServerConfigData::setClientBodySize(size_t client_body_size) 
{
    _cli_max_bodysize = client_body_size;
}

void ServerConfigData::addRoute(std::string route) 
{
    _routes.push_back(std::move(route));
}

const std::string& ServerConfigData::getHost() const {
    return _host;
}

const std::vector<std::string>& ServerConfigData::getPorts() const {
    return _ports;
}

const std::vector<std::string>& ServerConfigData::getRoutes() const {
    return _routes;
}

size_t ServerConfigData::getNumOfPorts() const {
    return _num_of_ports;
}

const std::string& ServerConfigData::getName() const {
    return _name;
}

const std::string& ServerConfigData::getErrorPage() const {
    return _error_page;
}

size_t ServerConfigData::getCliMaxBodysize() const {
    return _cli_max_bodysize;
}

void    ServerConfigData::printPorts() const
{
    for (const auto& port : _ports) {
        std::cout << port << "\n";
    }
} */