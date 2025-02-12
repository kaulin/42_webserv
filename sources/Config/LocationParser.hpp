#pragma once

#include "webserv.hpp"
#include <vector>
#include <unordered_map>
#include "ServerConfigData.hpp"
#include <unordered_map>

static const std::unordered_map<std::string, int> directiveMap = {
        {"methods", 1},
        {"autoindex", 2},
        {"redirect", 3},
        {"root", 4},
        {"index", 5},
        {"cgi_path", 6},
        {"cgi_param", 7}
};

class LocationParser {
    private:
        LocationParser() = delete;
        LocationParser(const LocationParser &other) = delete;
        LocationParser& operator=(const LocationParser &other) = delete;
    public:
        static std::tuple<std::string, Location>	set_location_block(std::vector<std::string>::const_iterator &iterator, 
												                        std::vector<std::string>::const_iterator &end,
												                        const std::unordered_map<std::string, Location> &locations);
		static std::unordered_map<std::string, bool>	set_location_methods(std::vector<std::string>::const_iterator &it);
		static std::pair<int, std::string>	set_redirect(std::vector<std::string>::const_iterator &it);
		static std::string					set_location_path(std::string path);
		static std::string	                set_root(std::vector<std::string>::const_iterator &it);
		static std::string	                set_index(std::vector<std::string>::const_iterator &it);
		static std::string	                set_cgi(std::vector<std::string>::const_iterator &it);
		static bool			                set_autoindex(std::vector<std::string>::const_iterator &it);
};