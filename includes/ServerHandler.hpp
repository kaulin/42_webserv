#pragma once

#include "webserv.hpp"
#include "HttpServer.hpp"
//#include "ServerConfigData.hpp"

class HttpServer;

class ServerHandler
{
private:
    std::vector<std::shared_ptr<HttpServer>>    _servers;
    size_t                      _server_count;
    std::vector<int>            _ports;
    std::vector<struct pollfd>  _pollfd_list;
    bool                         _running;
public:
    ServerHandler();
    ~ServerHandler();

    void    runServers();
    void    setupSockets();
    void    setupServers(std::string path);
    void    poll_loop();
    void    setPollList();
    void    error_and_exit(const char *msg);
    void    read_request(int new_sockfd);
    void    send_response(int sockfd_out);
    void    cleanupServers();
    size_t  getPortCount();

    // Helper functions for debugging
    // void    *get_in_addr(struct sockaddr *sa);
    // void    printServerData();
};
