#include "webserv.hpp"

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cout << "Error: Please provide configuration file\n";
		return 2;
	}
	if (argc > 2) {
		std::cout << "Error: Too many arguments\n";
		return 2;
	}
	const std::string conf = argv[1];
	std::cout << "Server started from configuration file: " << conf << "\n";
	return 0;
}