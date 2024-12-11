#include "webserv.hpp"
#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {}

ServerConfig::~ServerConfig() {}

void	ServerConfig::setConfigFilePath(std::string path)
{
	// check if valid file path, if not return error and exit
	_path = path;
}

/* Config file should set up the following:
	• Choose the port and host of each ’server’.
	• Setup the server_names or not.
	• The first server for a host:port will be the default for this host:port (that means
		it will answer to all the requests that don’t belong to any other server).
	• Setup default error pages.
	• Limit client body size.
	• Setup routes with one or multiple of the following rules/configuration:
		◦ Define a list of accepted HTTP methods for the route.
		◦ Define a HTTP redirection.
		◦ Define a directory or a file from where the file should be searched
			(for example, if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet 
			is /tmp/www/pouic/toto/pouet).
		◦ Turn on or off directory listing.
		◦ Set a default file to answer if the request is a directory.
		◦ Execute CGI based on certain file extension (for example .php).
		◦ Make it work with POST and GET methods.
		◦ Make the route able to accept uploaded files and configure where they should
			be saved.	
*/
void    ServerConfig::parseConfigFile(struct addrinfo serv_addr)
{
	int	status;
	struct addrinfo	data, *p, *server_info; 

	// Some temp data to test out server info where we allow incoming connections
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