#pragma once

//#include "webserv.hpp"
#include <map>
#include <vector>
#include <string>

struct Location {
	std::string _path;
	std::string _root;
	std::string _index;
	std::string _cgi_path;
	std::string _cgi_param;
	std::pair<int, std::string> _redirect;
	std::unordered_map<std::string, bool> _methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};
	bool		_dir_listing;
};

struct Config {
	std::string									_host;
	std::vector<std::string>					_names;
	std::vector<std::string>					_ports;
	std::unordered_map<std::string, Location>	_location;
	size_t										_num_of_ports;
	size_t										_cli_max_bodysize;
	std::map<int, std::string>					_default_pages;
	std::vector<std::string>					_error_pages;
	std::map<int, std::string>					_error_codes;
	std::map<std::string, std::string>			_cgi_params;
};


class ServerConfigData {
private:
	std::map<std::string, Config>   _serverConfigBlocks;
public:
	ServerConfigData(std::string path);
	ServerConfigData();
	~ServerConfigData();

	std::map<std::string, Config>&   getConfigBlocks();
	size_t  getServerCount();
	size_t  getPortCount();

	// class member functions
	void	printConfigs();
};