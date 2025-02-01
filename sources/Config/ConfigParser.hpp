#pragma once


#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include "webserv.hpp"


class ServerConfigData;

class HttpServer;
/* 
// Struct to hold location-specific configuration
struct LocationConfig
{
	std::string path;         // Path for the location (e.g., "/data")
	std::string root;         // Root directory for this location
	std::string index;        // Default index file (e.g., "index.html")
	bool autoindex = false;   // Whether autoindex is enabled
	std::string cgi_extension; // File extension for CGI (e.g., ".php")
	std::string cgi_handler;   // CGI handler program (e.g., "php-cgi")
};

// Struct to hold server-specific configuration
struct ServerConfig
{
	int listen = 80;                               // Port to listen on
	std::string root;                              // Root directory for the server
	std::unordered_map<int, std::string> error_pages; // Custom error pages (e.g., 404 -> "/errors/404.html")
	std::vector<LocationConfig> locations;        // List of location-specific configurations
	std::string	server_name;
};

// Struct to hold the global configuration
struct Config
{
	int workers = 1;                 // Number of worker threads or processes
	std::vector<ServerConfig> servers; // List of server configurations
};

// Class to parse the configuration file
class ConfigParser2
{
private:
	// Helper functions to parse specific parts of the configuration
	void parseLine(const std::string& line);
	void parseServerBlock();
	void parseLocationBlock(const std::string& line, ServerConfig& server);

	// Utility functions
	std::string trim(const std::string& str); // Trim whitespace
	int parseWorkers(const std::string& line);
	LocationConfig parseLocation(const std::string& line);
	void parseServerDirective(const std::string& line, ServerConfig& server);
	void parseLocationDirective(const std::string& line, LocationConfig& location);

	// State variables for parsing
	std::ifstream config_file_;  // File stream for the configuration file
	Config config_;              // Config object being constructed
	ServerConfig current_server_; // Current server block being parsed
	LocationConfig current_location_; // Current location block being parsed
	bool in_server_block_ = false; // Parsing inside a server block
	bool in_location_block_ = false; // Parsing inside a location block

public:
	// Constructor that takes the file path of the configuration file
	ConfigParser(const std::string& filepath);

	// Parses the configuration file and returns a Config object
	Config parse();

	const Config& getConfig() const;
}; */

struct ServerSection 
{
	std::unordered_map<std::string, std::vector<std::string>> configData;
};

class ConfigParser { 
private:
        ConfigParser() = delete;
        ConfigParser(const ConfigParser &other) = delete;
        ConfigParser& operator=(const ConfigParser &other) = delete;
public:
        // class member functions
        static std::map<std::string, std::vector<Config>> parseConfigFile(std::string path);
        static void     					checkConfigFilePath(std::string path);
		static std::string 					read_file(std::string path);
		static std::vector<ServerSection>	tokenize(std::string file_content);
		static std::vector<Location>		set_location_settings(ServerSection server);

};
