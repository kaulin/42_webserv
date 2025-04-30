#include "ServerConfigData.hpp"
#include "ConfigParser.hpp"

ServerConfigData::ServerConfigData()
{
	// use default server
}

ServerConfigData::ServerConfigData(std::string path) 
{
	// needs to set the server configs for each server
	// ServerConfigBlocks is of datastructure = std::map<std::string, std::vector<Config>>
	_serverConfigBlocks = ConfigParser::parseConfigFile(path);

	std::cout << "Server config data created based on: " << path << "\n";
}

ServerConfigData::~ServerConfigData() {
	_serverConfigBlocks.clear();
	std::cout << "Server config data deleted\n";
}

std::map<std::string, Config>&	ServerConfigData::getConfigBlocks()
{
	return (this->_serverConfigBlocks);
}

size_t  ServerConfigData::getServerCount()
{
	return _serverConfigBlocks.size();
}

/*
	Returns the Location struct for a specific path, if it exists, or nullptr, 
	if it doesn't.
*/
const Location* ServerConfigData::getLocation(const Config& config, std::string path) {
	if (path.size() > 1 && path.back() == '/')
			path.erase(path.begin() + path.find_last_of('/'), path.end());
	auto itLocation = config.locations.find(path);
	if (itLocation == config.locations.end())
		return nullptr;
	return &(*itLocation).second;
}

/*
	Returns the root of a specific path. If no Location setting is set for 
	current directory, looks at parent directories.
*/
const std::string& ServerConfigData::getRoot(const Config& config, std::string path){
	while(!path.empty())
	{
		if (path.size() > 1 && path.back() == '/')
			path.erase(path.begin() + path.find_last_of('/'), path.end());
		const Location* location = getLocation(config, path);
		if (location != nullptr)
			return location->root;
		path.erase(path.begin() + path.find_last_of('/') + 1, path.end());
	}
	return config.root;
}

/*
	Checks that the method requested is allowed for current directory or it's 
	parent directories.
*/
bool ServerConfigData::checkMethod(const Config& config, const std::string& method, std::string path) {
	while(!path.empty())
	{
		if (path.size() > 1 && path.back() == '/')
			path.erase(path.begin() + path.find_last_of('/'), path.end());
		const Location* location = getLocation(config, path);
		if (location != nullptr && location->methods.at(method))
			return true;
		if (path == "/")
			break;
		path.erase(path.begin() + path.find_last_of('/') + 1, path.end());
	}
	return false;
}