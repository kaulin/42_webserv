#include "ServerHandler.hpp"
#include "ConfigParser.hpp"

int main(int argc, char **argv) 
{
	if (argc > 2)
	{
		std::cerr << "Usage: " << std::string(argv[0]) << " [config file path]\n";
		return 2;
	}
	try
	{
		std::string configFilePath;
		configFilePath = (argc < 2) ? DEFAULT_CONFIG_FILE : argv[1];
		ConfigParser::checkConfigFilePath(configFilePath);
		ServerHandler serverHandler(configFilePath);
		serverHandler.setupServers();
		serverHandler.runServers(); // comment out when only testing config
	}
	catch (const ConfigParser::ConfigParserException& e)
	{
		std::cout << "Error: " << e.what() << '\n';
		return 1;
	}
	catch(const std::exception& e)
	{
		std::cerr << "Caught unexpected exception: " << e.what() << '\n';
	}
	return 0;
}