#include "webserv.hpp"

int main(int argc, char **argv) 
{
	std::string 		configFilePath;
	
	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	configFilePath = (argc < 2) ? DEFAULT_CONFIG_FILE : argv[1];
	try {
		ConfigParser::checkConfigFilePath(configFilePath);
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
	}
	try
	{
		ServerHandler serverHandler(configFilePath);
		try {
			serverHandler.setupServers();
		}
		catch(const std::exception& e) {
			std::cerr << e.what() << '\n';
		}
		serverHandler.runServers(); // comment out when only testing config
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return 0;
}