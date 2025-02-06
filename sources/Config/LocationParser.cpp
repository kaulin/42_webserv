#include "webserv.hpp"
#include "tuple"

int	LocationParser::validate_location_block(std::vector<std::string>::const_iterator &it)
{
	// check correct block structure and invalid data
	return (0);
}

std::string	LocationParser::set_location_path(std::string path)
{
	// 1. Regex based location
	// 2. Standard location block -> mapped to filesystem directory (root) ->  check existence
	// 3. Redirection -> target URL has to be available
	// 4. Upload directory -> check existence of root and has write permissions
}

std::pair<int, std::string>	LocationParser::set_redirect(std::vector<std::string>::const_iterator &it)
{
	return (std::pair<int, std::string> (std::stoi(*it), *(++it)));
}

std::string	LocationParser::set_root(std::vector<std::string>::const_iterator &it) { return (*(++it)); }

std::string	LocationParser::set_index(std::vector<std::string>::const_iterator &it) { return (*(++it)); }

std::string	LocationParser::set_cgi_path(std::vector<std::string>::const_iterator &it) { return (*(++it)); }

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
    std::string _cgi_extension;
    std::pair<int, std::string> _redirect;
    std::unordered_map<std::string, bool> _methods;
}; */

std::tuple<std::string, Location>	LocationParser::set_location_block(std::vector<std::string>::const_iterator &it, 
											std::vector<std::string>::const_iterator &end,
											const std::unordered_map<std::string, Location> &locations)
{
	Location location_block;

	if (it == end || validate_location_block(it))
		throw std::runtime_error("Invalid location URI/path");
	if (locations.find(*it) == locations.end()) // checks for duplicate
		throw std::runtime_error("Duplicate path");
	location_block._path = set_location_path(*it);
	for (; it != end && *it != "}";)
	{
		if (*it == "methods")
			location_block._methods = set_location_methods(it);
		if (*it == "redirect")
			location_block._redirect = set_redirect(it);
		if (*it == "root")
			location_block._root = set_root(it);
		if (*it == "index")
			location_block._index = set_index(it);
		if (*it == "cgi_path")
			location_block._cgi_path = set_cgi_path(it);
		it++;
	}
	return (std::tuple<std::string, Location>(location_block._path, location_block));
}