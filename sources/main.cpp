#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfig.hpp"

int main(int argc, char **argv) 
{
	HttpServer 		server;
	ServerConfig	config;
	struct addrinfo serv_addr;

	if (argc < 2) {
		std::cerr << "Error: Please provide configuration file\n";
		return 2;
	}
	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	config.setConfigFilePath(argv[1]);
	config.parseConfigFile(serv_addr);
	server.runServer(&serv_addr);
	return 0;
}