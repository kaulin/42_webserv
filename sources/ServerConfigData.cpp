#include "ServerConfigData.hpp"

ServerConfigData::ServerConfigData() 
{
    host.clear();
    name.clear();
    error_page.clear();
    routes.clear();
    port = -1;
    cli_max_bodysize = 0;
	std::cout << "New server config data created...: \n";
}

ServerConfigData::~ServerConfigData() {}