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
	ROOT,
	HOST,
	PORT,
	SERVER_NAME,
	ERROR_200,
	ERROR_201,
	ERROR_202,
	ERROR_204,
	ERROR_301,
	ERROR_307,
	ERROR_308,
	ERROR_400,
	ERROR_403,
	ERROR_404,
	ERROR_405,
	ERROR_406,
	ERROR_408,
	ERROR_411,
	ERROR_413,
	ERROR_414,
	ERROR_415,
	ERROR_418,
	ERROR_431,
	ERROR_500,
	ERROR_501,
	CLIENT_MAX_BODY_SIZE,
	CLIENT_TIMEOUT,
	UNKNOWN
};

const std::unordered_map<std::string, ConfigKey> keywordMap =
{
	{"location", ConfigKey::LOCATION},
	{"}", ConfigKey::END_BLOCK},
	{";", ConfigKey::SEMICOLON},
	{"root", ConfigKey::ROOT},
	{"host", ConfigKey::HOST},
	{"port", ConfigKey::PORT},
	{"server_name", ConfigKey::SERVER_NAME},
	{"error_page200", ConfigKey::ERROR_200},
	{"error_page201", ConfigKey::ERROR_201},
	{"error_page202", ConfigKey::ERROR_202},
	{"error_page204", ConfigKey::ERROR_204},
	{"error_page301", ConfigKey::ERROR_301},
	{"error_page307", ConfigKey::ERROR_307},
	{"error_page308", ConfigKey::ERROR_308},
	{"error_page400", ConfigKey::ERROR_400},
	{"error_page403", ConfigKey::ERROR_403},
	{"error_page404", ConfigKey::ERROR_404},
	{"error_page405", ConfigKey::ERROR_405},
	{"error_page406", ConfigKey::ERROR_406},
	{"error_page408", ConfigKey::ERROR_408},
	{"error_page411", ConfigKey::ERROR_411},
	{"error_page413", ConfigKey::ERROR_413},
	{"error_page414", ConfigKey::ERROR_414},
	{"error_page415", ConfigKey::ERROR_415},
	{"error_page418", ConfigKey::ERROR_418},
	{"error_page431", ConfigKey::ERROR_431},
	{"error_page500", ConfigKey::ERROR_500},
	{"error_page501", ConfigKey::ERROR_501},
	{"client_max_body_size", ConfigKey::CLIENT_MAX_BODY_SIZE},
	{"client_timeout", ConfigKey::CLIENT_TIMEOUT}
};

class ConfigParser { 
private:
	ConfigParser() = delete;
	ConfigParser(const ConfigParser &other) = delete;
	ConfigParser& operator=(const ConfigParser &other) = delete;
public:
	// class member functions
	static std::map<std::string, Config>	parseConfigFile(std::string path);
	static void								checkConfigFilePath(std::string path);
	static std::string						read_file(std::string path);
	static std::vector<std::string>			tokenize(std::string &file_content);
	static void								assignKeyToValue(std::vector<std::string>::const_iterator &it, std::vector<std::string>::const_iterator &end, Config &blockInstance);
	static void								assignErrorPage(std::vector<std::string>::const_iterator &it, std::vector<std::string>::const_iterator &end, Config &blockInstance, ConfigKey key);
	static void								setDefaultErrorPages(Config &blockInstance);
	static void								setRoot(Config *blockInstance);
	static void								checkDuplicates(std::map<std::string, Config> configs, Config *blockInstance);
	static void								checkRequired(Config *blockInstance);

	class ConfigParserException : public std::exception
		{
			private:
				const char *_message;
			public:
				ConfigParserException(const char * msg);
				const char* what() const throw();
		};
};
