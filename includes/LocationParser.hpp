#pragma once

#include <vector>
#include <unordered_map>
#include "ServerConfigData.hpp"

enum LocationConfigKey {
	METHODS,
	AUTOINDEX,
	REDIR,
	ROOT,
	INDEX,
	CGI_PATH,
	CGI_PARAM,
	BREAK
};

static const std::unordered_map<std::string, int> directiveMap = {
	{"methods", 0},
	{"autoindex", 1},
	{"return", 2}, // keyword for redirect
	{"root", 3},
	{"index", 4},
	{"cgi_path", 5},
	{"cgi_param", 6},
	{";", 7}
};

class LocationParser {
	private:
		LocationParser() = delete;
		LocationParser(const LocationParser &other) = delete;
		LocationParser& operator=(const LocationParser &other) = delete;
	public:
		static std::pair<std::string, Location>	set_location_block(std::vector<std::string>::const_iterator &iterator, 
		std::vector<std::string>::const_iterator &end,
		const std::unordered_map<std::string, Location> &locations);
		static void							set_location_methods(std::vector<std::string>::const_iterator &it, std::unordered_map<std::string, bool> methods);
		static std::pair<int, std::string>	set_redirect(std::vector<std::string>::const_iterator &it);
		static std::string					set_location_path(std::vector<std::string>::const_iterator &it);
		static std::string					set_root(std::vector<std::string>::const_iterator &it);
		static std::string					set_index(std::vector<std::string>::const_iterator &it);
		static std::string					set_cgi(std::vector<std::string>::const_iterator &it);
		static bool							set_autoindex(std::vector<std::string>::const_iterator &it);
};