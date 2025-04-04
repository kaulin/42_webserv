#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <string>

struct Location {
	std::string	_path;
	std::string	_root;
	std::string	_index;
	std::string	_cgi_path;
	std::string	_cgi_param;
	std::pair<int, std::string>	_redirect;
	std::unordered_map<std::string, bool>	_methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};
	bool		_dir_listing;
};

struct Config {
	std::string									_host;
	std::vector<std::string>					_names;
	std::string									_port;
	std::unordered_map<std::string, Location>	_location;
	size_t										_cli_max_bodysize;
	std::map<int, std::string>					_default_pages;
	std::vector<std::string>					_error_pages;
	std::map<int, std::string>					_error_codes;
};

class ServerConfigData {
private:
	std::map<std::string, Config>	_serverConfigBlocks;
public:
	ServerConfigData(std::string path);
	ServerConfigData();
	~ServerConfigData();

	std::map<std::string, Config>&   getConfigBlocks();
	size_t  getServerCount();
};
