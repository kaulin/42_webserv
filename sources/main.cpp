#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigParser.hpp"
#include "ServerConfigData.hpp"\

int main(int argc, char **argv) 
{
	std::string 		configFilePath;

	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	configFilePath = (argc < 2) ? DEFAULT_CONFIG_FILE : argv[1];
	ServerConfigParser	configParser;
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
	HttpServer	httpServer(configParser.servers[0]); // New server with the parsed config data, loop vector to set up all
	httpServer.setupAddrinfo();
	httpServer.runServer();
	return 0;
}