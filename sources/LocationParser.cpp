#include "LocationParser.hpp"
#include "ConfigParser.hpp"
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
		std::cout << "Invalid/No redirection code, using default" << '\n';
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

void LocationParser::verify_cgi(Location &location_block)
{
	size_t pos = location_block.cgiPath.find_last_of('.');
	if (pos == std::string::npos)
		throw ConfigParser::ConfigParserException("Invalid CGI path");
	std::string ext = location_block.cgiPath.substr(pos);
	if (ext != location_block.cgiExtension)
		throw ConfigParser::ConfigParserException("Unsupported CGI extension");
}

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
	location_block.dirListing = false;
	while (*it != "}")
	{
		auto found = directiveMap.find(*it);
		if (found == directiveMap.end() || it == end)
			throw ConfigParser::ConfigParserException("Invalid location block");
		switch (found->second)
		{
			case LocationConfigKey::METHODS:
				set_location_methods(it, location_block.methods);
				break;
			case LocationConfigKey::AUTOINDEX:
				location_block.dirListing = set_autoindex(it);
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
				location_block.cgiPath = set_cgi(it);
				break;
			case LocationConfigKey::CGI_PARAM:
				location_block.cgiParam = set_cgi(it);
				break;
			case LocationConfigKey::CGI_EXT:
				location_block.cgiExtension = set_cgi(it);
				break;
			case LocationConfigKey::BREAK:
				break;
			default:
				throw ConfigParser::ConfigParserException("Invalid directive"); // fatal
		}
		it++;
	}
	if (!location_block.cgiPath.empty() || !location_block.cgiExtension.empty())
		verify_cgi(location_block);
	return std::pair<std::string, Location>(location_block.path, location_block);
}