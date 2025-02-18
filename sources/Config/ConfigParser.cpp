#include "webserv.hpp"
#include "LocationParser.hpp"
#include "ConfigParser.hpp"

// helper trimming function
std::string trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos) return "";
	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
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

// function to read file, remove comments, return string
std::string ConfigParser::read_file(std::string path)
{
	std::ifstream file(path);
	std::stringstream content;
	std::string line;
	
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file " << path << std::endl;
		return "";
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
            {
				std::cerr << "Error: Invalid IP format: " << token << std::endl;
                return {};
            }
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
	for (const auto &token : tokens)
		std::cout << token << std::endl;

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
	{
		std::cerr << "Error: Missing '{' in config block.\n";
		return;
	}
	++it; // step over bracket

	blockInstance._num_of_ports = 0;
	// content starts here
	while (it != end)
	{
		auto keywordIt = keywordMap.find(*it);
		ConfigKey keyEnum = (keywordIt != keywordMap.end()) ? keywordIt->second : ConfigKey::UNKNOWN;

		switch (keyEnum)
		{
			case ConfigKey::LOCATION:
			{
				auto locationPair = LocationParser::set_location_block(it, end, blockInstance._location);
				blockInstance._location.emplace(locationPair.first, locationPair.second);
			}
				continue;
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
				while (it != end && *it != ";")
					{ blockInstance._ports.push_back(*it); blockInstance._num_of_ports++; ++it; }
				break;
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
				unsigned int mult = 1;
				std::string number = *it;
				if (!std::isdigit(number.back()))
				{
					if (number.back() == 'M')
					{
						mult = 1024 * 1024; // takes care of 'M', maybe implement a whole separate method for converting 'K', 'M', and 'G'...
						number.pop_back();
					}
				}
				blockInstance._cli_max_bodysize = std::stoul(number) * mult;
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
		throw std::runtime_error("Error: No file path provided");
	}
	if (path.find(".conf") == std::string::npos) {
		throw std::runtime_error("Error: Invalid file path");
	}
}

std::map<std::string, Config> ConfigParser::parseConfigFile(std::string path)
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


void	testPrintConfigs(std::map<std::string, Config> configs)
{
    for (const auto &config : configs)
    {
        std::cout << "Host: " << config.second._host << "\n"; 
        std::cout << "Names: ";
        for (const auto& name : config.second._names)
            std::cout << name << " ";
        std::cout << "\n";
        
        std::cout << "Ports: ";
        for (const auto& port : config.second._ports)
            std::cout << port << " ";
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
                std::cout << "    " << method.first << ": " << (method.second ? "Allowed" : "Not Allowed") << "\n";
            
            std::cout << "\n";
        }
    }
}

int main()
{
	std::map<std::string, Config> configs;

	configs = ConfigParser::parseConfigFile("config/test1.conf"); // insert here test config file to try

	testPrintConfigs(configs);
	return 0;
}