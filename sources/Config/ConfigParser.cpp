#include "webserv.hpp"
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
    
    while (ss >> token)
	{
        // do we need the semicolons?
        if (!token.empty() && token.back() == ';')
		{
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
		previous = token;
    }
    
    return tokens;
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
	std::map<std::string, Config>	configs;
	std::string 					file_data;
	std::vector<std::string> 		tokens; // tokens are saved in map with key value pairs -> [setting][vector:values]
	size_t							server_count = 0;

	file_data = read_file(path);
	tokens = tokenize(file_data);
	std::vector<std::string>::const_iterator it = tokens.begin();
	for (; it != tokens.end(); it++)
	{
		if (*it == "server") // a new Server Block Directive is encountered -> create new server config instance
		{
			Config blockInstance;

			// host...
			// ports... one has to be default
			// server_names...
			// error_pages...
			// limit client body size...
			if (*it == "location")
				blockInstance._location.insert(LocationParser::set_location_block(it, tokens.end(), blockInstance._location));
			// insert server block directive into vector holding 
			// configs["Server" + std::to_string(server_count++)] = {blockInstance};
			configs.insert({"Server" + std::to_string(server_count++), blockInstance});
		}
	}
	// print all configs
	return configs;
}
/* 
	ServerConfigData server_object;
	ServerConfigData server_object_2;
	std::vector<std::string> test_ports = {"3490", "3491"};
	std::vector<std::string> test_ports2 = {"8080"};
// void printServerConfigs(const std::vector<ServerConfigData>& serverConfigs) 
// {
// 	// for testing
// 	std::cout << "Printing all configs\n";
// 	for (const auto& conf : serverConfigs) {
// 		std::cout << "Host: " << conf.getHost() << "\n";
// 		conf.printPorts(); // helper function to print all ports
// 		std::cout << "Name: " << conf.getName() << "\n"
// 		<< "Error Page: " << conf.getErrorPage() << "\n"
// 		<< "Client Max Body Size: " << conf.getCliMaxBodysize() << "\n"
// 		<< "--------------------------\n";
// 	}
// }


// std::map<std::string, std::vector<Config>>    ConfigParser::parseConfigFile(std::string path)
// {
// 	std::string file_content = read_file(path);

// 	std::vector<std::string> tokens = tokenize(file_content);
// /* 
// 	ServerConfigData server_object;
// 	ServerConfigData server_object_2;
// 	std::vector<std::string> test_ports = {"3490", "3491"};
// 	std::vector<std::string> test_ports2 = {"8080"};

// 	for (auto& port : test_ports)
// 	{
// 		std::cout << "test ports " << port.c_str() << "\n";
// 	}
// 	for (auto& port :test_ports2)
// 	{
// 		std::cout << "test ports2 " << port.c_str() << "\n";
// 	}
// 	// adding some test data server 1 and server 2
// 	server_object.setHost("localhost");
// 	server_object.setPorts(test_ports);
// 	server_object.setServerName("example 1");
// 	server_object.setErrorPage("err.com");
// 	server_object.setClientBodySize(1024);
// 	// serverConfigs.push_back(server_object);

// 	server_object_2.setHost("localhost");
// 	server_object_2.setPorts(test_ports2);
// 	server_object_2.setServerName("example 2");
// 	server_object_2.setErrorPage("err.com");
// 	server_object_2.setClientBodySize(1024);
// 	// serverConfigs.push_back(server_object_2);
// 	// printServerConfigs(serverConfigs); */
// }
int main()
{
	std::string file_content = ConfigParser::read_file("../../config/test1.conf");

	std::vector<std::string> tokens = ConfigParser::tokenize(file_content);

	for (const auto &token : tokens)
	{
		std::cout << token << std::endl;
	}
	return 0;
}
