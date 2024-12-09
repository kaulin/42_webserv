#include "webserv.hpp"
#define PORT "3490" // the port users will be connecting to	


void	print_addresses(struct addrinfo *server_info)
{
	struct addrinfo *p;
	char *hostname;

	// JUST FOR TESTING: get hostname and Loop through all IP addresses in pointer
	std::cout << "Hostname and IP addresses:\n";
	if (gethostname(hostname, 255) == -1) {
		perror("Get host name");
		exit(errno);
	}
	for (char *temp = hostname; temp != NULL; temp++) {
		std::cout << temp << "\n";
	}
	for (p = server_info; p != NULL; p = p->ai_next)
	{
		void	*addr;
		char	*IP_version;
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			IP_version = "IPv4";
		} else {
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			IP_version = "IPv6";
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr); // changes the IP binary to printable
		std::cout << IP_version << " " << ipstr << "\n";
	}
}
/* Config file should set up the following:
	• Choose the port and host of each ’server’.
	• Setup the server_names or not.
	• The first server for a host:port will be the default for this host:port (that means
		it will answer to all the requests that don’t belong to any other server).
	• Setup default error pages.
	• Limit client body size.
	• Setup routes with one or multiple of the following rules/configuration:
		◦ Define a list of accepted HTTP methods for the route.
		◦ Define a HTTP redirection.
		◦ Define a directory or a file from where the file should be searched
			(for example, if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet 
			is /tmp/www/pouic/toto/pouet).
		◦ Turn on or off directory listing.
		◦ Set a default file to answer if the request is a directory.
		◦ Execute CGI based on certain file extension (for example .php).
		◦ Make it work with POST and GET methods.
		◦ Make the route able to accept uploaded files and configure where they should
			be saved.	
*/
void    parse_config_file(const std::string conf, struct addrinfo serv_addr)
{
	int	status;
	struct addrinfo	data, *p, *server_info; 

	// Some temp data to test out server info where we allow incoming connections
	std::memset(&data, 0, sizeof data); // set data to be empty
	data.ai_family = AF_UNSPEC; // IPv4 or IPv6
	data.ai_socktype = SOCK_STREAM; // TCP stream socket
	data.ai_flags = AI_PASSIVE; // auto fills IP address - sets to localhost's IP

	if ((status = getaddrinfo(NULL, PORT, &data, &server_info)) != 0) {
		std::cerr << gai_strerror(EXIT_FAILURE);
		exit(1);
	}
	print_addresses(server_info);
	// servinfo now points to a linked list of 1 or more struct addrinfos
}