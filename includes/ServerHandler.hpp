#pragma once
#include "webserv.hpp"

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
    void    pollLoop();
    void    setPollList();
    void    error_and_exit(const char *msg);
    void    readRequest(int new_sockfd);
    void    sendResponse(int sockfd_out);
    void    cleanupServers();
    size_t  getPortCount();
    void    signalHandler();

    // Helper functions for debugging
    // void    *get_in_addr(struct sockaddr *sa);
    // void    printServerData();
};
