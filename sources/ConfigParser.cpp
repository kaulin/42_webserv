#include "../includes/ConfigParser.hpp"

// Constructor: Opens the configuration file for reading
ConfigParser::ConfigParser(const std::string& filepath)
{
    config_file_.open(filepath);
    if (!config_file_.is_open())
        throw std::runtime_error("Failed to open configuration file: " + filepath);
}

// Trims leading and trailing whitespace from a string
std::string ConfigParser::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    return (first == std::string::npos || last == std::string::npos) 
            ? "" 
            : str.substr(first, last - first + 1);
}

// Parses the entire configuration file
Config ConfigParser::parse()
{
    std::string line;

    while (std::getline(config_file_, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        parseLine(line);
    }

    if (in_server_block_)
        config_.servers.push_back(current_server_);

    return config_;
}

// Parses an individual line of the configuration file
void ConfigParser::parseLine(const std::string& line)
{
    if (line == "server {")
    {
        if (in_server_block_)
            throw std::runtime_error("Nested server blocks are not allowed.");
        in_server_block_ = true;
        current_server_ = ServerConfig(); // Reset current server
    }
    else if (line == "}")
    {
        if (in_location_block_)
        {
            current_server_.locations.push_back(current_location_);
            in_location_block_ = false;
        }
        else if (in_server_block_)
        {
            config_.servers.push_back(current_server_);
            in_server_block_ = false;
        }
        else
            throw std::runtime_error("Unmatched closing brace.");
    }
    else if (in_server_block_)
    {
        if (line.find("location ") == 0)
            parseLocationBlock(line, current_server_);
        else if (in_location_block_)
            parseLocationDirective(line, current_location_);
        else
            parseServerDirective(line, current_server_);
    }
    else if (line.find("workers ") == 0)
    {
        config_.workers = parseWorkers(line);
    }
    else
        throw std::runtime_error("Unexpected line outside of server block: " + line);
}


// Parses a location block
void ConfigParser::parseLocationBlock(const std::string& line, ServerConfig& server)
{
    if (in_location_block_)
        throw std::runtime_error("Nested location blocks are not allowed.");
    in_location_block_ = true;

    size_t start = line.find(" ");
    size_t end = line.find(" {");
    if (start == std::string::npos || end == std::string::npos || start >= end)
        throw std::runtime_error("Invalid location directive: " + line);

    current_location_ = LocationConfig();
    current_location_.path = trim(line.substr(start, end - start));
}

LocationConfig ConfigParser::parseLocation(const std::string& line)
{
    size_t start = line.find(" ");
    size_t end = line.find(" ", start + 1);

    if (start == std::string::npos || end == std::string::npos)
        throw std::runtime_error("Invalid location directive: " + line);

    LocationConfig location;
    location.path = trim(line.substr(start, end - start)); // Extract the path
    location.cgi_extension = trim(line.substr(end + 1));   // Extract CGI extension and handler

    return location;
}

// Parses directives within a server block
void ConfigParser::parseServerDirective(const std::string& line, ServerConfig& server)
{
    if (line.find("listen ") == 0)
    {
        std::string port_str = trim(line.substr(7));  // Extract port as string
        try
        {
            int port = std::stoi(port_str);  // Convert the string to integer
            server.listen = port;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Invalid listen directive: " + line);
        }
    }
    else if (line.find("root ") == 0)
    {
        server.root = trim(line.substr(5));
    }
    else if (line.find("error_page ") == 0)
    {
        std::cout << "skipping error page" << std::endl;
        // size_t space = line.find(" ");
        // size_t second_space = line.find(" ", space + 1);
        // if (space == std::string::npos || second_space == std::string::npos)
        //     throw std::runtime_error("Invalid error_page directive: " + line);
        // int error_code = std::stoi(line.substr(10, space - 10));
        // std::string error_path = trim(line.substr(second_space));
        // server.error_pages[error_code] = error_path;
    }
    else if (line.find("server_name ") == 0) // Handle server_name directive
    {
        std::string server_names = trim(line.substr(12)); // Strip "server_name "
        std::istringstream ss(server_names);
        std::string name;
        while (ss >> name)
            server.server_name += name + " "; // Concatenate names (you can modify this to store them in a list)
        server.server_name = trim(server.server_name); // Remove any trailing spaces
    }
    else if (line.find("cgi ") == 0)
    {
        size_t space = line.find(" ");
        if (space == std::string::npos)
            throw std::runtime_error("Invalid cgi directive: " + line);
        server.locations.push_back(parseLocation(line)); // Add a CGI handler to a default location
    }
    else
    {
        throw std::runtime_error("Unknown directive in server block: " + line);
    }
}

// Parses directives within a location block
void ConfigParser::parseLocationDirective(const std::string& line, LocationConfig& location)
{
    if (line.find("root ") == 0)
        location.root = trim(line.substr(5));
    else if (line.find("index ") == 0)
        location.index = trim(line.substr(6));
    else if (line.find("autoindex ") == 0)
    {
        std::string value = trim(line.substr(10));

        // Remove semicolon if it exists
        if (!value.empty() && value.back() == ';') {
            value.pop_back(); // Remove the trailing semicolon
        }

        if (value == "on")
            location.autoindex = true;
        else if (value == "off")
            location.autoindex = false;
        else
            throw std::runtime_error("Invalid value for autoindex: " + value);
    }
    else if (line.find("cgi ") == 0)
    {
        size_t space = line.find(" ");
        if (space == std::string::npos)
            throw std::runtime_error("Invalid cgi directive: " + line);
        location.cgi_extension = trim(line.substr(4, space - 4));
        location.cgi_handler = trim(line.substr(space + 1));
    }
    else
        throw std::runtime_error("Unknown directive in location block: " + line);
}

int ConfigParser::parseWorkers(const std::string& line)
{
    try
    {
        return std::stoi(trim(line.substr(8))); // Extract the number after "workers "
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Invalid workers directive: " + line);
    }
}

const Config& ConfigParser::getConfig() const { return config_; }
