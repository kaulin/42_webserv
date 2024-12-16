#include "ServerConfigParser.hpp"

ServerConfigParser::ServerConfigParser () {
	_servers = nullptr;
	_path = nullptr;
}

ServerConfigParser::~ServerConfigParser() {}

void	ServerConfigParser::setConfigFilePath(std::string path)
{
	// check if valid file path, if not return error and exit
	if (path.empty()) {
		throw std::runtime("Error: No file path provided");
	}
	_path = path;
}

void    ServerConfigParser::parseConfigFile()
{
	int	status;
	struct addrinfo	data, *p, *server_info; 

	// Some temp data to test out server info where we allow incoming connections
	// should loop through each server config and create a new Server Data object
	std::memset(&data, 0, sizeof data); // set data to be empty
	data.ai_family = AF_UNSPEC; // IPv4 or IPv6
	data.ai_socktype = SOCK_STREAM; // TCP stream socket
	data.ai_flags = AI_PASSIVE; // auto fills IP address - sets to localhost's IP
	if ((status = getaddrinfo(NULL, "80", &data, &server_info)) != 0) {
		std::cerr << gai_strerror(EXIT_FAILURE);
		exit(1);
	}
	// servinfo now points to a linked list of 1 or more struct addrinfos
}