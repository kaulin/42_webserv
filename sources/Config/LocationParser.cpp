#include "webserv.hpp"
#include "tuple"
#include "LocationParser.hpp"
#include <regex>

std::string	LocationParser::set_location_path(std::string path)
{
	std::regex locationPattern(R"(^\/([A-Za-z0-9._-]+\/?)+$)");
	if (std::regex_match(path, locationPattern))
		return path;
	else
		throw std::runtime_error("Invalid location path format");
}

std::pair<int, std::string>	LocationParser::set_redirect(std::vector<std::string>::const_iterator &it)
{
	return std::pair<int, std::string> (std::stoi(*it), *(++it));
}

std::string	LocationParser::set_root(std::vector<std::string>::const_iterator &it) { return *(++it); }

std::string	LocationParser::set_index(std::vector<std::string>::const_iterator &it) { return *(++it); }

std::string	LocationParser::set_cgi(std::vector<std::string>::const_iterator &it) 
{ 
	return *(++it); 
}

bool LocationParser::set_autoindex(std::vector<std::string>::const_iterator &it) 
{
	return *(++it) == "on" ? true : false;
}

std::unordered_map<std::string, bool>	LocationParser::set_location_methods(std::vector<std::string>::const_iterator &it)
{
	std::unordered_map<std::string, bool> methods = {{"GET", false}, {"POST", false}, {"DELETE", false}};

	for (; *it != ";" ; it++)
	{
		if (*it == "GET")
			methods["GET"] = true;
		if (*it == "POST")
			methods["POST"] = true;
		if (*it == "DELETE")
			methods["DELETE"] = true;
	}
	return (methods);
}

/* struct Location {
    bool        _dir_listing;
    std::string _path;
    std::string _root;
    std::string _index;
    std::string _cgi_path;
    std::string _cgi_param;
    std::pair<int, std::string> _redirect;
    std::unordered_map<std::string, bool> _methods;
}; */

std::tuple<std::string, Location>	LocationParser::set_location_block(std::vector<std::string>::const_iterator &it, 
											std::vector<std::string>::const_iterator &end,
											const std::unordered_map<std::string, Location> &locations)
{
	Location location_block;

	if (it == end)
		throw std::runtime_error("Invalid location URI/path");
	if (locations.find(*it) == locations.end())
		throw std::runtime_error("Duplicate path");
	location_block._path = set_location_path(*it);
	for (; *it != "}";)
	{
		auto found = directiveMap.find(*it);
		if (found == directiveMap.end() || it == end)
			throw std::runtime_error("Invalid location block");

		switch (found->second)
		{
			case 1:
				location_block._methods = set_location_methods(it);
				break;
			case 2:
				location_block._dir_listing = set_autoindex(it);
				break;
			case 3:
				location_block._redirect = set_redirect(it);
				break;
			case 4:
				location_block._root = set_root(it);
				break;
			case 5:
				location_block._index = set_index(it);
				break;
			case 6:
				location_block._cgi_path = set_cgi(it);
				break;
			case 7:
				location_block._cgi_param = set_cgi(it);
				break;
			default:
				throw std::runtime_error("Invalid directive"); // fatal
		}
		it++;
	}
	return (std::tuple<std::string, Location>(location_block._path, location_block));
}