#include "webserv.hpp"
#include "LocationParser.hpp"
#include "ConfigParser.hpp"
#include <filesystem>


void	testPrintConfigs(std::map<std::string, Config> configs)
{
	for (const auto &config : configs)
	{
		std::cout << "Host: " << config.second._host << "\n"; 
		std::cout << "Names: ";
		for (const auto& name : config.second._names)
			std::cout << name << " ";
		std::cout << "\n";
		
		std::cout << "Port: ";
		std::cout << config.second._port << " ";
		std::cout << "\n";
		
		std::cout << "Number of Ports: " << config.second._num_of_ports << "\n";
		std::cout << "Client Max Body Size: " << config.second._cli_max_bodysize << "\n";
		
		std::cout << "Default Pages:\n";
		for (const auto& page : config.second._default_pages)
			std::cout << "  " << page.first << ": " << page.second << "\n";
		
		std::cout << "Error Pages:\n";
		for (const auto& page : config.second._error_pages)
			//std::cout << "  " << page.first << ": " << page.second << "\n";
			std::cout << page << " ";
		
		std::cout << "Error Codes:\n";
		for (const auto& code : config.second._error_codes)
			std::cout << "  " << code.first << ": " << code.second << "\n";
		
		std::cout << "CGI Params:\n";
		for (const auto& param : config.second._cgi_params)
			std::cout << "  " << param.first << " = " << param.second << "\n";
		std::cout << "---------------------\n";
		std::cout << "Locations:\n";
		for (const auto& loc : config.second._location) {
			const Location& location = loc.second;
			std::cout << "  Path: " << location._path << "\n";
			std::cout << "  Root: " << location._root << "\n";
			std::cout << "  Index: " << location._index << "\n";
			std::cout << "  CGI Path: " << location._cgi_path << "\n";
			std::cout << "  CGI Param: " << location._cgi_param << "\n";
			std::cout << "  Redirect: " << location._redirect.first << " -> " << location._redirect.second << "\n";
			std::cout << "  Directory Listing: " << (location._dir_listing ? "Enabled" : "Disabled") << "\n";
			
			std::cout << "  Methods:\n";
			for (const auto& method : location._methods)
				std::cout << "	" << method.first << ": " << (method.second ? "Allowed" : "Not Allowed") << "\n";
			
			std::cout << "\n";
		}
	}
	std::cout << "Finished printing configs\n";
}

// helper trimming function
std::string trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos) return "";
	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
}

// helper function to validate port
bool isValidPort(const std::string &port)
{
	int num;
	try
	{
		num = std::stoi(port);
	}
	catch (const std::out_of_range&)
	{
		throw ConfigParser::ConfigParserException("Config: Int overflow in port.");
	}
	std::string::const_iterator it = port.begin();
    while (it != port.end() && std::isdigit(*it))
		++it;
    return !port.empty() && it == port.end();
}

// helper function to validate IP address
bool isValidIP(const std::string &ip)
{
	std::regex ipRegex(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");
	std::smatch match;
	if (std::regex_match(ip, match, ipRegex))
	{
		for (int i = 1; i <= 4; ++i)
		{
			int octet = std::stoi(match[i].str());
			if (octet < 0 || octet > 255)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

// helper function to convert orders of magnitude if client max body size is presented in kilobytes, megabytes or gigabytes
size_t convertMaxClientSize(const std::string& number)
{
    char magnitude = number.back();
    size_t mult = 1;

    if (magnitude == 'K' || magnitude == 'k')
        mult = 1024;
    else if (magnitude == 'M' || magnitude == 'm')
        mult = 1024 * 1024;
    else if (magnitude == 'G' || magnitude == 'g')
        mult = 1024 * 1024 * 1024;
    else if (!std::isdigit(magnitude))
        throw std::invalid_argument("Invalid client max body size suffix");

    return mult;
}


// function to read file, remove comments, return string
std::string ConfigParser::read_file(std::string path)
{
	std::string fullPath;
	std::filesystem::path filePath(path);

	if (!filePath.is_absolute()) {
		fullPath = filePath;
	}
	else {
		fullPath = std::filesystem::absolute(filePath);
	}

	std::ifstream file(fullPath);
	std::stringstream content;
	std::string line;
	
	if (!file.is_open())
	{
		ConfigParserException("Config: Could not open config file.");
	}
	
	while (std::getline(file, line))
	{
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
		{
			line = line.substr(0, commentPos);
		}
		
		line = trim(line);
		if (!line.empty())
		{
			content << line << '\n';
		}
	}
	
	file.close();
	return content.str();
}

// Function to parse the configuration file
std::vector<std::string> ConfigParser::tokenize(std::string &file_content)
{
	std::vector<std::string> tokens;
	std::stringstream ss(file_content);
	std::string token, previous;
	bool semicolon = false;

	while (ss >> token)
	{
		semicolon = false;
		if (!token.empty() && token.back() == ';')
		{
			semicolon = true;
			token.pop_back();
		}
		if (token == "{" || token == "}")
		{
			tokens.push_back(token);
			continue;
		}
		if (previous == "host")
		{
			if (!isValidIP(token))
				throw ConfigParserException("Config: Invalid host IP address.");
		}
		if (previous == "port")
		{
			if (!isValidPort(token))
				throw ConfigParserException("Config: Invalid port.");
		}
		if (previous == "error_page")
		{
			tokens.pop_back();
			token = previous + token;
		}
		tokens.push_back(token);
		if (semicolon)
			tokens.push_back(";");
		previous = token;
	}
	return tokens;
}


void ConfigParser::assignKeyToValue(std::vector<std::string>::const_iterator &it, 
	std::vector<std::string>::const_iterator &end,
	Config &blockInstance)
{
	// find opening bracket
	while (it != end && *it != "{")
	++it;
	if (it == end)
		throw ConfigParserException("Config: Missing '{' in config block.");
	++it; // step over bracket

	// content starts here
	while (it != end)
	{
		auto keywordIt = keywordMap.find(*it);
		ConfigKey keyEnum = (keywordIt != keywordMap.end()) ? keywordIt->second : ConfigKey::UNKNOWN;

		switch (keyEnum)
		{
			case ConfigKey::LOCATION:
			{
				auto locationPair = LocationParser::set_location_block(++it, end, blockInstance._location);
				blockInstance._location.emplace(locationPair.first, locationPair.second);
				++it;
				continue;
			}
			case ConfigKey::END_BLOCK:
				return;
			case ConfigKey::SEMICOLON:
				++it;
				continue;
			case ConfigKey::HOST:
				++it;
				blockInstance._host = *it;
				++it; break;
			case ConfigKey::PORT:
				++it;
				blockInstance._port = *it; 
				++it; break;
			case ConfigKey::SERVER_NAME:
				++it;
				while (it != end && *it != ";")
					{ blockInstance._names.push_back(*it); ++it; }
				break;
			case ConfigKey::ERROR_404:
				++it;
				blockInstance._error_pages.push_back(*it);
				++it; break;
			case ConfigKey::ERROR_500:
				++it;
				blockInstance._error_pages.push_back(*it);
				++it; break;
			case ConfigKey::CLIENT_MAX_BODY_SIZE:
			{
				++it;
				size_t mult = 1;
				std::string number = *it;
				if (!std::isdigit(number.back()))
				{
					mult = convertMaxClientSize(number);
					number.pop_back();
				}
				size_t bodySize;
				try
				{
					bodySize = std::stoul(number);
				}
				catch (const std::out_of_range&)
				{
					throw ConfigParserException("Config: Max client body size overflow.");
				}
				if (bodySize > SIZE_MAX / mult)
					throw ConfigParserException("Config: Max client body size overflow.");
				blockInstance._cli_max_bodysize = bodySize * mult;
				++it; break; 
			}
			case ConfigKey::UNKNOWN:
				break; // default handling
		}
		++it; // iterates main loop in case of default handling
	}
}


void	ConfigParser::checkConfigFilePath(std::string path)
{
	if (path.empty()) {
		throw ConfigParserException("Config: No file path provided.");
	}
	if (path.find(".conf") == std::string::npos) {
		throw ConfigParserException("Config: Invalid file path.");
	}
}

std::map<std::string, Config> ConfigParser::parseConfigFile(std::string path)
{
	try
	{
		std::map<std::string, Config> configs;
		size_t server_count = 0;
		std::string file_content = read_file(path);
		std::vector<std::string> tokens = tokenize(file_content);

		std::vector<std::string>::const_iterator it = tokens.begin();
		std::vector<std::string>::const_iterator end = tokens.end();

		while (it != end)
		{
			if (*it == "server")
			{
				Config blockInstance; // new server block
				
				assignKeyToValue(++it, end, blockInstance);
				
				configs.insert({"Server" + std::to_string(server_count++), blockInstance});
			}
			++it;
		}
		return configs;
	}
	catch (ConfigParserException e)
	{
		std::cout << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}

// config parser specific exception
ConfigParser::ConfigParserException::ConfigParserException(const char *msg) : _message(msg) {}

const char* ConfigParser::ConfigParserException::what() const throw()
{
	if (_message)
		return _message;
	else
		return "Error: Config parser exception.";
}