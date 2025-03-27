#include "webserv.hpp"

int main(int argc, char **argv) 
{
	std::string configFilePath;
	
	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	try
	{
		configFilePath = (argc < 2) ? DEFAULT_CONFIG_FILE : argv[1];
		ConfigParser::checkConfigFilePath(configFilePath);
		ServerHandler serverHandler(configFilePath);
			serverHandler.setupServers();
		serverHandler.runServers(); // comment out when only testing config
	}
	catch (const ConfigParser::ConfigParserException& e)
	{
		std::cout << e.what() << '\n';
		return 1;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return 0;
}