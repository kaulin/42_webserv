#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "../includes/ConfigParser.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "use path to config file as argument" << std::endl;
        return (1);
    }
    try
    {
        ConfigParser parser(argv[1]);
        Config config = parser.parse();

        // Output parsed configuration
        std::cout << "Workers: " << config.workers << "\n";
        for (const auto& server : config.servers)
        {
            std::cout << "Server Name: " << server.server_name << "\n";
            std::cout << "Listen: " << server.listen << "\n";
            std::cout << "Root: " << server.root << "\n";
            for (const auto& [code, path] : server.error_pages)
                std::cout << "Error Page [" << code << "]: " << path << "\n";
            for (const auto& location : server.locations)
            {
                std::cout << "Location Path: " << location.path << "\n";
                std::cout << "Root: " << location.root << "\n";
                std::cout << "Index: " << location.index << "\n";
                std::cout << "Autoindex: " << (location.autoindex ? "on" : "off") << "\n";
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
    }
    return 0;
}
