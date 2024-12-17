#include "ServerConfigData.hpp"
#include "ServerConfigParser.hpp"

ServerConfigParser::ServerConfigParser () {
	_path = "";
	servers.clear();
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

void printServerConfigs(const std::vector<ServerConfigData>& servers) {
    for (const auto& server : servers) {
        std::cout << "Host: " << server.host << "\n"
                  << "Port: " << server.port << "\n"
                  << "Name: " << server.name << "\n"
                  << "Error Page: " << server.error_page << "\n"
                  << "Client Max Body Size: " << server.cli_max_bodysize << "\n"
                  << "--------------------------\n";
    }
}

void    ServerConfigParser::parseConfigFile()
{
	// TO DO: parse here and throw error if config file has issues
	if (this->_path == "") {
		throw std::runtime_error("Error: Error in config file");
	}
	// loop to set all the servers config data
	ServerConfigData server_object;
	// adding some test data
	server_object.host = "localhost";
	server_object.port = 8080;
	server_object.name = "example";
	server_object.error_page = "err.com";
	server_object.cli_max_bodysize = 1000;

	servers.push_back(server_object);
	printServerConfigs(servers);
}