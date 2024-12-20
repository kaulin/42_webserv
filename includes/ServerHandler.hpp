#pragma once

#include "webserv.hpp"
#include "HttpServer.hpp"

class HttpServer;

class ServerHandler
{
private:
    std::vector<HttpServer> _servers;
    size_t                  _server_count;
public:
    ServerHandler();
    ~ServerHandler();

    void    runServers();
    void    setupServers(std::vector<ServerConfigData> server);
    void    poll_loop();
    void    error_and_exit(const char *msg);
    void    handle_request(int sockfd);
    void    send_response(int sockfd_out, std::string response);
    void    *get_in_addr(struct sockaddr *sa);
    void    printServerData();
};
