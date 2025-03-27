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