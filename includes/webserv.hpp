#ifndef WEBSERV_HPP
#define WEBSERV_HPP

# include <iostream>
# include <cstring>
# include <sys/socket.h>
# include <sys/types.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <poll.h>
# include <sys/wait.h>
# include <sys/select.h>
# include <poll.h>
# include <errno.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <vector>
# include <iostream>
# include <memory>
#include <utility>

# define DEFAULT_CONFIG_FILE "config/default.conf"
# define CONFIG_DIRECTORY "webserv/config/"

// #include "HttpServer.hpp"
// #include "ConfigParser.hpp"
// #include "ServerConfigData.hpp"
// #include "ServerHandler.hpp"

// ------------ testing with relative paths ---------------
#include "../sources/Config/ConfigParser.hpp"
#include "../sources/Config/ServerConfigData.hpp"
// #include "ServerConfigData.hpp"
// #include "ConfigParser.hpp"
#include "../sources/Config/LocationParser.hpp"
#include "HttpServer.hpp"
#include "ServerHandler.hpp"


#endif