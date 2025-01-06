#pragma once

#include "webserv.hpp"
#include "HttpServer.hpp"
#include "ServerConfigData.hpp"

class HttpServer;

struct pollfd_obj {
    struct pollfd   *poll_fds;
    size_t          num_of_ports;
    int             listen_sockfd;
};

class ServerHandler
{
private:
    std::vector<HttpServer>     _servers;
    size_t                      _server_count;
    std::vector<int>            _ports;
    std::vector<struct pollfd> _pollfd_list;
public:
    ServerHandler();
    ~ServerHandler();

    void    runServers();
    void    setupServers(std::vector<ServerConfigData> server);
    void    poll_loop();
    void    setPollList();
    void    error_and_exit(const char *msg);
    void    handle_request(int new_sockfd);
    void    send_response(int sockfd_out);
    void    *get_in_addr(struct sockaddr *sa);
    void    printServerData();
};
