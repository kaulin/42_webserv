#include <iostream>
#include <cstring> // For memset
#include <netdb.h> // For getaddrinfo and freeaddrinfo
#include <sys/socket.h> // For AF_INET, SOCK_STREAM
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h> // For htons (if needed)


std::string convertBinaryToIPv4(uint32_t ip_binary)
{
	// Convert from network byte order to host byte order
	ip_binary = ntohl(ip_binary);

	// Extract each byte from the 32-bit integer
	uint8_t octet1 = (ip_binary >> 24) & 0xFF;
	uint8_t octet2 = (ip_binary >> 16) & 0xFF;
	uint8_t octet3 = (ip_binary >> 8) & 0xFF;
	uint8_t octet4 = ip_binary & 0xFF;

	// Build the string representation
	char ip_str[16]; // Maximum length for IPv4 address "xxx.xxx.xxx.xxx"
	snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", octet1, octet2, octet3, octet4);

	return std::string(ip_str);
}

bool resolveDNS(const std::string& hostname, std::string& ip_address)
{
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // Only IPv4
	hints.ai_socktype = SOCK_STREAM; // Stream socket (e.g., TCP)

	// Perform DNS resolution
	int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
	if (status != 0)
	{
		std::cerr << "DNS resolution failed: " << gai_strerror(status) << std::endl;
		return false;
	}

	// Extract the IPv4 address from the first result
	struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
	uint32_t ip_binary = ipv4->sin_addr.s_addr; // Binary IP in network byte order

	// Convert binary IP to human-readable format (manual implementation of inet_ntop)
	ip_address = convertBinaryToIPv4(ip_binary);

	// Free the linked list
	freeaddrinfo(res);
	return true;
}

int main(void)
{
	std::string hostname = "google.com";
	std::string ip_address;

	if (resolveDNS(hostname, ip_address))
		std::cout << "Resolved " << hostname << " to IP: " << ip_address << std::endl;
	else
		std::cerr << "Failed to resolve hostname: " << hostname << std::endl;
	return 0;
}
