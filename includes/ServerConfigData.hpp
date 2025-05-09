#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <string>

struct Location {
	std::string	path;
	std::string	root;
	std::string	index;
	std::string	cgiPath;
	std::string cgiExtension;
	std::string	cgiParam;
	std::pair<int, std::string>	redirect;
	std::unordered_map<std::string, bool>	methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};
	bool		dirListing;
};

struct Config {
	std::string									root;
	std::string									host;
	std::vector<std::string>					names;
	std::string									port;
	std::unordered_map<std::string, Location>	locations;
	size_t										cliMaxBodysize;
	int											timeout;
	std::map<int, std::string>					defaultPages;
	std::map<int, std::string>					errorPages;
};

class ServerConfigData {
private:
	std::map<std::string, Config>	_serverConfigBlocks;
public:
	ServerConfigData(std::string path);
	ServerConfigData();
	~ServerConfigData();

	std::map<std::string, Config>&	getConfigBlocks();
	size_t  getServerCount();

	static const Location* getLocation(const Config& config, std::string path);
	static const std::string& getRoot(const Config& config, std::string path);
	static bool checkMethod(const Config& config, const std::string& method, std::string path);
};