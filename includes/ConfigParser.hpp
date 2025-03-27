#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include "ServerConfigData.hpp"
#include <algorithm>
#include <regex>

class ServerConfigData;

class HttpServer;

enum class ConfigKey
{
	LOCATION,
	END_BLOCK,
	SEMICOLON,
	HOST,
	PORT,
	SERVER_NAME,
	ERROR_404,
	ERROR_500,
	CLIENT_MAX_BODY_SIZE,
	UNKNOWN
};

const std::unordered_map<std::string, ConfigKey> keywordMap =
{
	{"location", ConfigKey::LOCATION},
	{"}", ConfigKey::END_BLOCK},
	{";", ConfigKey::SEMICOLON},
	{"host", ConfigKey::HOST},
	{"port", ConfigKey::PORT},
	{"server_name", ConfigKey::SERVER_NAME},
	{"error_page404", ConfigKey::ERROR_404},
	{"error_page500", ConfigKey::ERROR_500},
	{"client_max_body_size", ConfigKey::CLIENT_MAX_BODY_SIZE}
};

class ConfigParser { 
private:
	ConfigParser() = delete;
	ConfigParser(const ConfigParser &other) = delete;
	ConfigParser& operator=(const ConfigParser &other) = delete;
public:
	// class member functions
	static std::map<std::string, Config>	parseConfigFile(std::string path);
	static void							checkConfigFilePath(std::string path);
	static std::string						read_file(std::string path);
	static std::vector<std::string>			tokenize(std::string &file_content);
	static void								assignKeyToValue(std::vector<std::string>::const_iterator &it, std::vector<std::string>::const_iterator &end, Config &blockInstance);

	class ConfigParserException : public std::exception
		{
			private:
				const char *_message;
			public:
				ConfigParserException(const char * msg);
				const char* what() const throw();
		};
};
