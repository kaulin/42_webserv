#include "ServerHandler.hpp"

/* void	ServerHandler::printServerData()
{
	std::vector<int> listensockfds;
	// for testing
	for (auto& server : _servers) {
        std::cout << "Host: " << server.getName() << "\n";
		listensockfds = server.getListenSockfds();
		for (auto& curr : listensockfds) {
			std::cout << "Listen sockfd: " << curr << "\n";
		}
        std::cout << "Number of ports: " << server.getNumOfPorts()
        << "\n--------------------------\n";
    }
} 

void	*ServerHandler::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

*/

void	ServerHandler::printPollFds()
{
	for (auto& poll_obj : _pollfd_list) 
	{
		std::cout << "Polling on fd: " << poll_obj.fd << "\n";
	}
}

size_t	ServerHandler::getPortCount()
{
	size_t	num_of_ports = 0;

	for (auto& server : _servers) {
		num_of_ports += server->getNumOfPorts();
	}
	std::cout << "Number of ports total: " << num_of_ports << "\n";
	return (num_of_ports);
}