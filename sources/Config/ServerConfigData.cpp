#include "webserv.hpp"
#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"

ServerConfigData::ServerConfigData()
{
	// use default server
}

ServerConfigData::ServerConfigData(std::string path) 
{
	// needs to set the server configs for each server
	// ServerConfigBlocks is of datastructure = std::map<std::string, std::vector<Config>>
	_serverConfigBlocks = ConfigParser::parseConfigFile(path);
/*	 
	_host.clear();
	_name.clear();
	_routes.clear();
	_ports.clear();
	_cli_max_bodysize = 0;
	_location.clear(); */

	std::cout << "New server config data created...: \n";
}

ServerConfigData::~ServerConfigData() {
	std::cout << "Server config data instance deleted\n";
}

std::map<std::string, Config>&	ServerConfigData::getConfigBlocks()
{
	return (this->_serverConfigBlocks);
}

size_t  ServerConfigData::getServerCount()
{
	return _serverConfigBlocks.size();
}