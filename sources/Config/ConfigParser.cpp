#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"
#include "webserv.hpp"
#include <string>
#include <iostream>
#include <unistd.h>
#include <fctntl.h>

ConfigParser::~ConfigParser() {}

void	ConfigParser::checkConfigFilePath(std::string path)
{
	// check that format for relevant info is correct etc, throw error if not
	if (path.empty()) {
		throw std::runtime_error("Error: No file path provided");
	}
	if (path.find(".conf") == std::string::npos) {
		throw std::runtime_error("Error: Invalid file path");
	}
}

void printServerConfigs(const std::vector<ServerConfigData>& serverConfigs) 
{
    // for testing
	std::cout << "Printing all configs\n";
	for (const auto& conf : serverConfigs) {
        std::cout << "Host: " << conf.getHost() << "\n";
		conf.printPorts(); // helper function to print all ports
        std::cout << "Name: " << conf.getName() << "\n"
        << "Error Page: " << conf.getErrorPage() << "\n"
        << "Client Max Body Size: " << conf.getCliMaxBodysize() << "\n"
        << "--------------------------\n";
    }
}

std::map<std::string, std::vector<Config>>    ConfigParser::parseConfigFile(std::string path)
{
	if (open(path.c_str(), O_RDONLY) == -1) {
		throw std::runtime_error("Error: Could not open config file");
	}
	// loop to set all the servers config data
	ServerConfigData server_object;
	ServerConfigData server_object_2;
	std::vector<std::string> test_ports = {"3490", "3491"};
	std::vector<std::string> test_ports2 = {"8080"};

	for (auto& port : test_ports)
	{
		std::cout << "test ports " << port.c_str() << "\n";
	}
	for (auto& port :test_ports2)
	{
		std::cout << "test ports2 " << port.c_str() << "\n";
	}
	// adding some test data server 1 and server 2
	server_object.setHost("localhost");
	server_object.setPorts(test_ports);
	server_object.setServerName("example 1");
	server_object.setErrorPage("err.com");
	server_object.setClientBodySize(1024);
	// serverConfigs.push_back(server_object);

	server_object_2.setHost("localhost");
	server_object_2.setPorts(test_ports2);
	server_object_2.setServerName("example 2");
	server_object_2.setErrorPage("err.com");
	server_object_2.setClientBodySize(1024);
	// serverConfigs.push_back(server_object_2);
	// printServerConfigs(serverConfigs);
}