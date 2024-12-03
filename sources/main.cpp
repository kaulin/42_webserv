#include "webserv.hpp"

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "Error: Please provide configuration file\n";
		return 2;
	}
	if (argc > 2) {
		std::cerr << "Error: Too many arguments\n";
		return 2;
	}
	struct addrinfo serv_addr;
	const std::string conf = argv[1];
	parse_config_file(conf, serv_addr);
	run_server(serv_addr);
	std::cerr << "Server started from configuration file: " << conf << "\n";
	freeaddrinfo(&serv_addr); // free the pointers alloc'd by getaddrinfo
	return 0;
}