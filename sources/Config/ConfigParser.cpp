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
	bool		openingBracket = false;
	
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

std::unordered_map<std::string, std::vector<std::string>>	ConfigParser::assignKeyToValue(std::vector<std::string> &tokens,
																				std::vector<std::string>::iterator &it)
{
	std::unordered_map<std::string, std::vector<std::string>> configMap; // use map<string, vector<string>> ?
	std::string key;
	std::string keytype;
	std::string value;
	bool isValue = false;
	int num = 1;

	std::cout << "1: " << *it << std::endl;
	while (it < tokens.end() && *it != "{")
		*it++;
	*it++;
	std::cout << "2: " << *it << std::endl;
	while (it < tokens.end())
	{
		if (*it == "server")
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
		configMap[key] = value;
		std::cout << "3: " << *it << std::endl;
	}
	return configMap;
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
	std::map<std::string, Config>	configs; // map containing all vitual server configurations
	size_t							server_count = 0;
	std::string 					file_content = read_file("../../config/test1.conf");
	std::vector<std::string>		tokens = tokenize(file_content);

	std::vector<std::string>::const_iterator it = tokens.begin();
	std::vector<std::string>::const_iterator end = tokens.end();


	for (; it != end; it++)
	{
		if (*it == "server") // a new Server Block Directive is encountered -> create new server config instance
		{
			Config blockInstance;
			
			std::unordered_map<std::string, std::string> configMap = assignKeyToValue(tokens, ++it); // pass in here the config block instead and set data
			if (*it == "location")
				blockInstance._location.emplace(LocationParser::set_location_block(it, end, blockInstance._location));
			configs.insert({"Server" + std::to_string(server_count++), blockInstance});
		}
	}
	return configs;
}

int main()
{

	std::vector<std::string>::iterator it = tokens.begin();

	for (const auto &token : tokens)
	{
		std::cout << token << std::endl;
	}

	std::cout << "Keys in config map:\n";
	for (const auto &[key, _] : configMap)
	{
		std::cout << key << std::endl;
	}

	std::cout << "Some values in config map:\n";
	std::cout << "host: " << configMap["host"] << std::endl;
	std::cout << "server_name: " << configMap["server_name"] << std::endl;
	std::cout << "server_name2: " << configMap["server_name2"] << std::endl;
	std::cout << "root: " << configMap["root"] << std::endl;
	std::cout << "route: " << configMap["route"] << std::endl;
	return 0;
}