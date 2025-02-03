#include "webserv.hpp"
#include <string>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"

void	ConfigParser::checkConfigFilePath(std::string path)
{
	if (path.empty()) {
		throw std::runtime_error("Error: No file path provided");
	}
	if (path.find(".conf") == std::string::npos) {
		throw std::runtime_error("Error: Invalid file path");
	}
}

/* void printServerConfigBlocks(const std::vector<ServerConfigData>& ServerConfigBlocks) 
{
	// for testing
	std::cout << "Printing all configs\n";
	for (const auto& conf : ServerConfigBlocks) {
		std::cout << "Host: " << conf.getHost() << "\n";
		conf.printPorts(); // helper function to print all ports
		std::cout << "Name: " << conf.getName() << "\n"
		<< "Error Page: " << conf.getErrorPage() << "\n"
		<< "Client Max Body Size: " << conf.getCliMaxBodysize() << "\n"
		<< "--------------------------\n";
	}
} */

std::vector<Location>	ConfigParser::set_location_context(std::vector<std::string> token, int index)
{
	Location location_settings;


}

std::vector<std::string>    ConfigParser::tokenize(std::string file_data)
{
	std::vector<std::string> tokens;

	// 1. tokenise
	// 2. sets the Server sections --> 
	/* 
		vector of BlockDirectives {Server1, Server2, Server3...}, 
		where each server holds BlockDirective -> which is an unordered map with key pairs eg.
			server_name : {value1, value2, value3}
			client_max_body_size : {10M}
			error_page : {404, /errors/404.html}
			error_page : {500, /errors/500.html}
			routes : {route, "/", "{", root, /var/www/html, ";", methods, GET ...}
	 
		return parsed sections
	*/
	return tokens;
}

std::string ConfigParser::read_file(std::string path)
{
	std::string file_content;

	// "Read file" implementation here...
	if (open(path.c_str(), O_RDONLY) == -1) {
		throw std::runtime_error("Error: Could not open config file");
	}
	// Returns read content with comments and whitespace removed
	return file_content;
}

std::map<std::string, std::vector<Config>>    ConfigParser::parseConfigFile(std::string path)
{
	std::map<std::string, std::vector<Config>> configs;
	std::string file_data;
	std::vector<std::string> tokens; // tokens are saved in map with key value pairs -> [setting][vector:values]

	file_data = read_file(path);
	tokens = tokenize(file_data);
	for (int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == "server") // a new server block is encountered
		{
			Config serverBlockInstance;
			std::vector<Config> blockDirectives;
			// SET config directives
			serverBlockInstance._location = set_location_context(tokens, i);
			configs.insert({"Server" + std::to_string(i), blockDirectives});
		}

		i++;
	}
	return configs;
/* 
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
	// ServerConfigBlocks.push_back(server_object);

	server_object_2.setHost("localhost");
	server_object_2.setPorts(test_ports2);
	server_object_2.setServerName("example 2");
	server_object_2.setErrorPage("err.com");
	server_object_2.setClientBodySize(1024);
	// ServerConfigBlocks.push_back(server_object_2);
	// printServerConfigBlocks(ServerConfigBlocks); */
}
