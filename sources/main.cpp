#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigParser.hpp"

int main(int argc, char **argv) 
{
	std::string 		configFilePath;
	ServerConfigParser	configParser;

	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	configFilePath = (argc < 2) ? DEFAULT_CONFIG_FILE : argv[1];
	try
	{
		configParser.setConfigFilePath(configFilePath);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	try
	{
		configParser.parseConfigFile(); // after parsing there is a vector of serverData objects that hold all the server info
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	HttpServer	server(configParser._servers); // New server with the parsed config data
	server.setupAddrinfo();
	server.runServer();
	return 0;
}