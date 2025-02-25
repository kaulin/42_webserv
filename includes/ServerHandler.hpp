#pragma once
#include "webserv.hpp"

class HttpServer;

class ServerHandler
{
private:
    std::vector<std::shared_ptr<HttpServer>>    _servers;
    size_t                      _server_count;
    std::vector<int>            _ports;
    std::vector<struct pollfd>  _pollFds;
    bool                         _running;
    ServerConfigData            _config;
public:
    ServerHandler(std::string path);
    ~ServerHandler();

    void    runServers();
    void    setupSockets();
    void    setupServers();
    void    pollLoop();
    void    setPollList();
    void    error_and_exit(const char *msg);
    void    readRequest(int new_sockfd);
    void    sendResponse(int sockfd_out);
    void    cleanupServers();
    size_t  getPortCount();

    static void    signalHandler(int);

    // Helper functions for debugging
    void	printPollFds();
    // void    *get_in_addr(struct sockaddr *sa);
    // void    printServerData();
};
