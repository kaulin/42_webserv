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

# define DEFAULT_CONFIG_FILE "config/default.conf"

#include "HttpServer.hpp"
#include "ServerHandler.hpp"
#include "ConfigParser.hpp"
#include "ServerConfigData.hpp"

#endif