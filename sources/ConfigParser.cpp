#include "LocationParser.hpp"
#include "ConfigParser.hpp"
#include "DNS.hpp"
#include <filesystem>


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
	(void)num;
	try
	{
		num = std::stoi(port);
		if (num < 1 || num > 65535)
			return false;
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
bool isValidIP(std::string &ip)
{
	std::regex ipRegex(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");
	std::smatch match;
	std::string ip_address;
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
	else if (DNS::resolveDNS(ip, ip_address))
	{
		ip = ip_address;
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

int parseTimeout(const std::string& value) {
    std::regex timeRegex(R"((\d+)(s|m|h))");
    std::smatch match;

    if (std::regex_match(value, match, timeRegex))
	{
		try
		{
			int time = std::stoi(match[1]);
			std::string unit = match[2];

			if (unit == "s") return time;
			if (unit == "m") return time * 60;
			if (unit == "h") return time * 60 * 60;
		}
		catch (...)
		{
			throw ConfigParser::ConfigParserException("Config: Timeout value out of range.");
		}
    }
    throw ConfigParser::ConfigParserException("Config: Invalid timeout format.");
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
		throw ConfigParserException("Config: Could not open config file.");
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
	int opening = 0;
	int closing = 0;
	for (auto &token : tokens)
	{
		if (token == "{")
			opening++;
		if (token == "}")
			closing++;
	}
	if (opening != closing)
		throw ConfigParserException("Config: Mismatched brackets in config file.");
	// for (auto token : tokens)
	// 	std::cout << token << std::endl;
	return tokens;
}

bool isErrorCode(ConfigKey key) { return key >= ConfigKey::ERROR_200 && key <= ConfigKey::ERROR_501; }

void ConfigParser::assignErrorPage(std::vector<std::string>::const_iterator &it, 
	std::vector<std::string>::const_iterator &end,
	Config &blockInstance, ConfigKey key)
{
	static const std::map<ConfigKey, int> keyToCode = {
		{ ConfigKey::ERROR_200, 200 },
		{ ConfigKey::ERROR_201, 201 },
		{ ConfigKey::ERROR_202, 202 },
		{ ConfigKey::ERROR_204, 204 },
		{ ConfigKey::ERROR_301, 301 },
		{ ConfigKey::ERROR_307, 307 },
		{ ConfigKey::ERROR_308, 308 },
		{ ConfigKey::ERROR_400, 400 },
		{ ConfigKey::ERROR_403, 403 },
		{ ConfigKey::ERROR_404, 404 },
		{ ConfigKey::ERROR_405, 405 },
		{ ConfigKey::ERROR_406, 406 },
		{ ConfigKey::ERROR_408, 408 },
		{ ConfigKey::ERROR_411, 411 },
		{ ConfigKey::ERROR_413, 413 },
		{ ConfigKey::ERROR_414, 414 },
		{ ConfigKey::ERROR_415, 415 },
		{ ConfigKey::ERROR_418, 418 },
		{ ConfigKey::ERROR_431, 431 },
		{ ConfigKey::ERROR_500, 500 },
		{ ConfigKey::ERROR_501, 501 },
		{ ConfigKey::ERROR_504, 504 },
	};

	auto itCode = keyToCode.find(key);
	if (itCode != keyToCode.end()) {
		++it;
		if (it == end)
			throw ConfigParserException("Config: Missing path for error code.");
		blockInstance.errorPages.emplace(itCode->second, *it);
		++it;
	}
}

void ConfigParser::setDefaultErrorPages(Config &blockInstance)
{
	static const std::map<int, std::string> defaultPages = {
		{301, "/errors/301.html"},
		{307, "/errors/307.html"},
		{308, "/errors/308.html"},
		{400, "/errors/400.html"},
		{403, "/errors/403.html"},
		{404, "/errors/404.html"},
		{405, "/errors/405.html"},
		{406, "/errors/406.html"},
		{408, "/errors/408.html"},
		{411, "/errors/411.html"},
		{413, "/errors/413.html"},
		{414, "/errors/414.html"},
		{415, "/errors/415.html"},
		{418, "/errors/418.html"},
		{431, "/errors/431.html"},
		{500, "/errors/500.html"},
		{501, "/errors/501.html"},
		{502, "/errors/502.html"},
		{503, "/errors/503.html"},
		{504, "/errors/504.html"}
	};

	for (const auto& entry : defaultPages)
	{
		if (blockInstance.errorPages.find(entry.first) == blockInstance.errorPages.end())
			blockInstance.errorPages[entry.first] = entry.second;
	}
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
		if (*it == "server")
			throw ConfigParserException("Config: Server block cannot contain another server block.");
		auto keywordIt = keywordMap.find(*it);
		ConfigKey keyEnum = (keywordIt != keywordMap.end()) ? keywordIt->second : ConfigKey::UNKNOWN;
		
		if (isErrorCode(keyEnum))
		{
			assignErrorPage(it, end, blockInstance, keyEnum);
			continue;
		}
		switch (keyEnum)
		{
			case ConfigKey::LOCATION:
			{
				auto locationPair = LocationParser::set_location_block(++it, end, blockInstance.locations);
				blockInstance.locations.emplace(locationPair.first, locationPair.second);
				++it;
				continue;
			}
			case ConfigKey::END_BLOCK:
				return;
			case ConfigKey::SEMICOLON:
				++it;
				continue;
			case ConfigKey::ROOT:
				++it;
				blockInstance.root = *it;
				++it; break;
			case ConfigKey::HOST:
				++it;
				blockInstance.host = *it;
				++it; break;
			case ConfigKey::PORT:
				++it;
				blockInstance.port = *it; 
				++it; break;
			case ConfigKey::SERVER_NAME:
				++it;
				while (it != end && *it != ";")
					{ blockInstance.names.push_back(*it); ++it; }
				break;
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
				blockInstance.cliMaxBodysize = bodySize * mult;
				++it; break; 
			}
			case ConfigKey::CLIENT_TIMEOUT:
			{
				++it;
				blockInstance.timeout = parseTimeout(*it);
				++it; break;
			}
			case ConfigKey::UNKNOWN:
				std::cout << "Config: Skipped over unknown directive: " << *it << std::endl;
				break; // default handling
			default:
				break;
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

void	ConfigParser::setRoot(Config *blockInstance)
{
	if (blockInstance->root.empty())
		blockInstance->root = "var/www/html";
	for (auto& loc : blockInstance->locations)
	{
		if (loc.second.root.empty())
			loc.second.root = blockInstance->root;
	}
}

void ConfigParser::checkDuplicates(std::map<std::string, Config> configs, Config *blockInstance)
{
	for (auto& config : configs)
	{
		if (config.second.host == blockInstance->host)
		{
			if (config.second.port == blockInstance->port)
				throw ConfigParserException("Config: Duplicate ports not allowed.");
		}
	}
}	

void ConfigParser::checkRequired(Config *blockInstance)
{
	if (blockInstance->host.empty())
		throw ConfigParserException("Config: No host set in config file.");
	if (blockInstance->root.empty())
		throw ConfigParserException("Config: No root set in config file.");
	if (blockInstance->port.empty())
		throw ConfigParserException("Config: No port set in config file.");
}

std::map<std::string, Config> ConfigParser::parseConfigFile(std::string path)
{
	std::map<std::string, Config> configs;
	size_t server_count = 0;
	std::string file_content = read_file(path);
	std::vector<std::string> tokens = tokenize(file_content);

	std::vector<std::string>::const_iterator it = tokens.begin();
	std::vector<std::string>::const_iterator end = tokens.end();
	if (std::find(tokens.begin(), tokens.end(), "server") == tokens.end())
		throw ConfigParserException("Config: No 'server' block in config file.");

	while (it != end)
	{
		if (*it == "server")
		{
			Config blockInstance; // new server block

			blockInstance.timeout = 0;
			assignKeyToValue(++it, end, blockInstance);
			setDefaultErrorPages(blockInstance);
			setRoot(&blockInstance);
			
			checkRequired(&blockInstance);
			checkDuplicates(configs, &blockInstance);
			configs.insert({"Server" + std::to_string(server_count++), blockInstance});
		}
		++it;
	}
	return configs;
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