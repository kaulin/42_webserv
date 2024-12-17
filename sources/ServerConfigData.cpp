#include "ServerConfigData.hpp"

ServerConfigData::ServerConfigData() 
{
    host = nullptr;
    port = -1;
    name = nullptr;
    error_page = nullptr;
    cli_max_bodysize = 0;
    routes.empty();
}

ServerConfigData::~ServerConfigData() {}