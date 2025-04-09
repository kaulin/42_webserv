#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <string>

struct Location {
	std::string	path;
	std::string	root;
	std::string	index;
	std::string	cgi_path;
	std::string	cgi_param;
	std::pair<int, std::string>	redirect;
	std::unordered_map<std::string, bool>	methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};
	bool		_dir_listing;
};

struct Config {
	std::string									root;
	std::string									host;
	std::vector<std::string>					names;
	std::string									port;
	std::unordered_map<std::string, Location>	locations;
	size_t										cli_max_bodysize;
	std::map<int, std::string>					default_pages;
	std::map<int, std::string>					error_pages;
	//std::map<int, std::string>					error_codes; // unnecessary?
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
};
