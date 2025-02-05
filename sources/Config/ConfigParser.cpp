#include "webserv.hpp"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"
#include <unordered_map>

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

int	ConfigParser::validate_location_block(std::vector<std::string>::const_iterator &it)
{
	// check correct block structure and invalid data
	return (0);
}

std::string	ConfigParser::set_location_path(std::string path)
{
	// 1. Regex based location
	// 2. Standard location block -> mapped to filesystem directory (root) ->  check existence
	// 3. Redirection -> target URL has to be available
	// 4. Upload directory -> check existence of root and has write permissions
}

std::pair<int, std::string>	ConfigParser::set_redirect(std::vector<std::string>::const_iterator &it)
{
	int 		code = std::stoi(*it);
	std::string redir = *(++it);

	return (std::pair<int, std::string> (code, redir));
}

std::string	ConfigParser::set_root(std::vector<std::string>::const_iterator &it)
{

}

std::string	ConfigParser::set_index(std::vector<std::string>::const_iterator &it)
{
	
}

std::string	ConfigParser::set_cgi_path(std::vector<std::string>::const_iterator &it)
{
	
}

std::unordered_map<std::string, bool>	ConfigParser::set_location_methods(std::vector<std::string>::const_iterator &it)
{
	std::unordered_map<std::string, bool> methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};

	for (; *it != ";" ; it++)
	{
		if (*it == "GET")
			methods["GET"] = true;
		if (*it == "POST")
			methods["POST"] = true;
		if (*it == "DELETE")
			methods["DELETE"] = true;
	}
	return (methods);
}

/* struct Location {
    std::string _path;
    std::string _root;
    std::string _index;
    std::string _cgi_path;
    std::string _cgi_extension;
    std::pair<int, std::string> _redirect;
    std::unordered_map<std::string, bool> _methods;
    bool        _dir_listing;
}; */

Location	ConfigParser::set_location_block(std::vector<std::string>::const_iterator &it, 
											std::vector<std::string>::const_iterator &end,
											const std::unordered_map<std::string, Location> &locations)
{
	Location location_block;

	if (it == end || validate_location_block(it))
		throw std::runtime_error("Invalid location URI/path");
	if (locations.find(*it) == locations.end()) // checks for duplicate
		throw std::runtime_error("Duplicate path");
	location_block._path = set_location_path(*it);
	for (; it != end && *it != "}";)
	{
		if (*it == "methods")
			location_block._methods = set_location_methods(it);
		if (*it == "redirect")
			location_block._redirect = set_redirect(it);
		if (*it == "root")
			location_block._root = set_root(it);
		if (*it == "index")
			location_block._index = set_index(it);
		if (*it == "cgi_path")
			location_block._cgi_path = set_cgi_path(it);
		it++;
	}
	return (location_block);
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

std::map<std::string, Config>    ConfigParser::parseConfigFile(std::string path)
{
	std::map<std::string, Config>	configs;
	std::string 					file_data;
	std::vector<std::string> 		tokens; // tokens are saved in map with key value pairs -> [setting][vector:values]
	size_t							server_count = 0;

	file_data = read_file(path);
	tokens = tokenize(file_data);
	std::vector<std::string>::const_iterator it = tokens.begin();
	for (; it != tokens.end(); it++)
	{
		if (*it == "server") // a new Server Block Directive is encountered -> create new server config instance
		{
			Config blockInstance;

			// SET config directives
			// host...
			// ports...
			if (*it == "location")
				set_location_block(it, tokens.end(), blockInstance._location);
			// insert server block directive into vector holding 
			// configs["Server" + std::to_string(server_count++)] = {blockInstance};
			configs.insert({"Server" + std::to_string(server_count++), blockInstance});
		}
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
