#include "../include/Webserv.hpp"

bool validKey(std::string key) {
    const std::string validKeys[] = {
    "port", "server_names","host", "allowed_methods",
    "max_body_size", "error_pages", "route"};

    for (int i = 0; i < 7; i++) {
        if (key == validKeys[i])
            return true;
    }
    return false;
}
int isspace3(int c) {
    return c == ' ' || c == '\t';
}

Config parseSeverBlock(std::string& server_block) {
    Config ServerConfig;

    std::string token;
    server_block.erase(std::remove_if(server_block.begin(), server_block.end(), isspace3), server_block.end());
    std::istringstream tokenStream(server_block);
    while (std::getline(tokenStream, token, '\n')) {
        if (token.empty()) continue;
    
        int pos = token.find("=");
        if (pos == std::string::npos)
            throw std::runtime_error("SYNTAX ERROR: missing = in configuration");

        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);
        if (validKey(key) == 0)
            throw std::runtime_error("SYNTAX ERROR: invalid configuration key");
        if (*(value.end() - 1) != ';')
            throw std::runtime_error("SYNTAX ERROR: missing ; in configuration");
        value = value.substr(0, value.size() - 1);
        if (value.empty())
            throw std::runtime_error("SYNTAX ERROR: empty value in configuration");
        if  (key == "port") { ServerConfig.insertPort(value); }
        else if (key == "host") { ServerConfig.insertHost(value); }
        else if (key == "allowed_methods") { ServerConfig.insertAllowedMethods(value); }
        else if (key == "server_names") { ServerConfig.insertServerNames(value); }
        else if (key == "max_body_size") { ServerConfig.insertMaxBodySize(value); }
        else if (key == "error_pages") { ServerConfig.insertErrorPages(value); }
        else if (key == "route") { ServerConfig.insertRoute(value); }
    }
    return ServerConfig;
}

std::vector<Config> parseConfigFile(std::string configFile) {
    Config ServerConfig;
    std::vector<Config> serverPool;

    size_t start = 0;
    size_t end = 0;
    while ((start = configFile.find("SERVER = [", start)) != std::string::npos) {
        start+= 10;
        end = configFile.find("]", start);
        if (end == std::string::npos)
            throw std::runtime_error("missing closing bracket");
        std::string server_block = configFile.substr(start, end - start);
        // add syntax check later..
        if (server_block.find("[") != std::string::npos)
            throw std::runtime_error("SYNTAX ERROR: nested server block");
        ServerConfig = parseSeverBlock(server_block);
        serverPool.push_back(ServerConfig);
        start = end;
    }
    return (serverPool);
}

int main(int ac, char **av) {
    std::string config_path = (ac > 1) ? av[1] : "webserv.conf";
    std::string configFile;
    try {
        configFile = readConfigFile(config_path);
        ServerManager serverManager(parseConfigFile(configFile));
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}