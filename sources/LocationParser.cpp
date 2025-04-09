#include "LocationParser.hpp"
#include <tuple>
#include <regex>
#include <utility>
#include <iostream>

std::string	LocationParser::set_location_path(std::vector<std::string>::const_iterator &it)
{
	std::string path = *it;
	std::regex locationPattern(R"(^\/([A-Za-z0-9._-]+\/?)*$)");

	if (std::regex_match(path, locationPattern))
	{
		it += 2;
		return path;
	}
	else
		throw std::runtime_error("Invalid location path format");
}

std::pair<int, std::string>	LocationParser::set_redirect(std::vector<std::string>::const_iterator &it)
{
	int redir_code = 301;

	try
	{
		redir_code = std::stoi(*(++it));
	}
	catch(const std::exception& e) 
	{
		std::cerr << "Invalid/No redirection code, using default" << '\n';
	}
	return std::pair<int, std::string> (redir_code, *(++it));
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

void	LocationParser::set_location_methods(std::vector<std::string>::const_iterator &it, std::unordered_map<std::string, bool>& methods)
{
	for (; *it != ";" ; it++)
	{
		if (*it == "GET")
			methods["GET"] = true;
		if (*it == "POST")
			methods["POST"] = true;
		if (*it == "DELETE")
			methods["DELETE"] = true;
	}
}

/* struct Location {
	bool		_dir_listing;
	std::string path;
	std::string root;
	std::string index;
	std::string cgi_path;
	std::string cgi_param;
	std::pair<int, std::string> redirect;
	std::unordered_map<std::string, bool> methods;
}; */

std::pair<std::string, Location>	LocationParser::set_location_block(std::vector<std::string>::const_iterator &it, 
	std::vector<std::string>::const_iterator &end,
	const std::unordered_map<std::string, Location> &locations)
{
	Location location_block;


	if (it == end)
		throw std::runtime_error("Invalid location URI/path");
	if (locations.find(*it) != locations.end())
		throw std::runtime_error("Duplicate path");
	location_block.path = set_location_path(it);
	while (*it != "}")
	{
		auto found = directiveMap.find(*it);
		if (found == directiveMap.end() || it == end)
			throw std::runtime_error("Invalid location block");
		switch (found->second)
		{
			case LocationConfigKey::METHODS:
				set_location_methods(it, location_block.methods);
				break;
			case LocationConfigKey::AUTOINDEX:
				location_block.dir_listing = set_autoindex(it);
				break;
			case LocationConfigKey::REDIR:
				location_block.redirect = set_redirect(it);
				break;
			case LocationConfigKey::ROOT:
				location_block.root = set_root(it);
				break;
			case LocationConfigKey::INDEX:
				location_block.index = set_index(it);
				break;
			case LocationConfigKey::CGI_PATH:
				location_block.cgi_path = set_cgi(it);
				break;
			case LocationConfigKey::CGI_PARAM:
				location_block.cgi_param = set_cgi(it);
				break;
			case LocationConfigKey::BREAK:
				break;
			default:
				throw std::runtime_error("Invalid directive"); // fatal
		}
		it++;
	}
	return std::pair<std::string, Location>(location_block.path, location_block);
}