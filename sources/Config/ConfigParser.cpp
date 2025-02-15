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
std::vector<std::string>    ConfigParser::tokenize(std::string &file_content)
{
	std::vector<std::string> tokens;
	std::stringstream ss(file_content);
	std::string token, previous;
	bool        semicolon;
	// bool		openingBracket = false;
	
	while (ss >> token)
	{
		semicolon = false;
		if (!token.empty() && token.back() == ';')
		{
			semicolon = true;
			token.pop_back();
		}
		if (previous == "host")
		{
			if (!isValidIP(token))
			{
				std::cerr << "Error: Invalid IP format: " << token << std::endl;
				return {};
			}
		}
		tokens.push_back(token);
		if (semicolon)
			tokens.push_back(";");
		previous = token;
	}
	
	return tokens;
}

void ConfigParser::assignKeyToValue(std::vector<std::string>::const_iterator &it, std::vector<std::string>::const_iterator &end,
									Config blockInstance)
{
	std::unordered_map<std::string, std::vector<std::string>> configMap;
	std::string key;
	std::string keytype;
	std::string value;
	bool isValue = false;
	int num = 1;

	std::cout << "1: " << *it << std::endl;
	while (it < end && *it != "{")
		*it++;
	*it++;
	std::cout << "2: " << *it << std::endl;
	// this could use switch statements and for loop
	while (it < end)
	{
		if (*it == "location") // sets location
				blockInstance._location.emplace(LocationParser::set_location_block(it, end, blockInstance._location));
		if (*it == "}") // this checks when the server block ends ("server is checked in calling function")
			break; 
		if (*it == ";")
		{
			isValue = false;
			*it++;
			continue;
		}
		if (!isValue)
		{
			key = *it;
			keytype = key;
			isValue = true;
			num = 1;
		}
		if (*it == key)
			value = *++it;
		else
		{
			key = keytype + std::to_string(++num);
			value = *it++;
		}
		configMap[key].emplace_back(value); // changed to emplace because of vector
		std::cout << "3: " << *it << std::endl;
	}
	// Here the parsed values should be set into blockInstance (which is a struct Config datatype)
	// returns (void) to calling method and comes back here when another "server" block is encountered
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

std::map<std::string, Config>    ConfigParser::parseConfigFile(std::string path)
{
	std::map<std::string, Config>	configs; // map containing all virtual server configurations
	size_t							server_count = 0;
	std::string 					file_content = read_file(path);
	std::vector<std::string>		tokens = tokenize(file_content);

	std::vector<std::string>::const_iterator it = tokens.begin();
	std::vector<std::string>::const_iterator end = tokens.end();

	for (; it != end; it++)
	{
		if (*it == "server") // a new Server Block Directive is encountered -> create new server config instance
		{
			Config blockInstance;
			
			assignKeyToValue(++it, end, blockInstance); // pass in here the config block and set data -> returns at the end of server block
			configs.insert({"Server" + std::to_string(server_count++), blockInstance}); // adds the blockInstance to configs
		}
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
            std::cout << "  " << page.first << ": " << page.second << "\n";
        
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

	configs = ConfigParser::parseConfigFile("../../config/test1.conf"); // insert here test config file to try

	testPrintConfigs(configs);
	return 0;
}