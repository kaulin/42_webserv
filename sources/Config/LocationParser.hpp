#pragma once

#include "webserv.hpp"


class LocationParser {
    private:
        LocationParser() = delete;
        LocationParser(const LocationParser &other) = delete;
        LocationParser& operator=(const LocationParser &other) = delete;
    public:
        static Location		set_location_block(std::vector<std::string>::const_iterator &iterator, 
												std::vector<std::string>::const_iterator &end,
												const std::unordered_map<std::string, Location> &locations);
		static std::unordered_map<std::string, bool>	set_location_methods(std::vector<std::string>::const_iterator &it);
		static std::pair<int, std::string>	set_redirect(std::vector<std::string>::const_iterator &it);
		static std::string					set_location_path(std::string path);
		static std::string	                set_root(std::vector<std::string>::const_iterator &it);
		static std::string	                set_index(std::vector<std::string>::const_iterator &it);
		static std::string	                set_cgi_path(std::vector<std::string>::const_iterator &it);
		static int							validate_location_block(std::vector<std::string>::const_iterator &it);
};