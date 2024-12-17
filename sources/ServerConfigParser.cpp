#include "ServerConfigParser.hpp"
#include "ServerConfigData.hpp"

ServerConfigParser::ServerConfigParser () {
	_path = nullptr;
	_servers.empty();
}

ServerConfigParser::~ServerConfigParser() {}

void	ServerConfigParser::setConfigFilePath(std::string path)
{
	// check if valid file path, if not return error and exit
	if (path.empty()) {
		throw std::runtime_error("Error: No file path provided");
	}
	_path = path;
}

void    ServerConfigParser::parseConfigFile()
{
	// parse here and throw error if config file has issues
	if (this->_path == "") {
		throw std::runtime_error("Error: Error in config file");
	}
	int	i = 0;

	// loop to set all the servers config data
	ServerConfigData *server_object = new ServerConfigData();
	std::memset(&_servers[i], 0, sizeof (_servers));
	
	// adding some test data
	server_object->host = "localhost";
	server_object->port = 8080;
	server_object->name = "example";
	server_object->error_page = "err.com";
	server_object->cli_max_bodysize = 1000;

	_servers[i] = *server_object;
}