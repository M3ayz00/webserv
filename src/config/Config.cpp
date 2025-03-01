#include "../../include/Config.hpp"

Config::Config() : max_body_size(1048576) {} // 1MB


std::string readConfigFile(std::string path) {
    std::string config_content;
    if (path.size() < 5 || (path[path.size() - 1] != 'f' || path[path.size() - 2] != 'n') || path[path.size() - 3] != 'o' || path[path.size() - 4] != 'c' || path[path.size() - 5]  != '.')
        throw std::runtime_error("invalid extention, expected exemple.conf");
    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + path);
    std::string line;
    while (std::getline(file, line))
        config_content += line + "\n";
    if (config_content.size() == 0)
        throw std::runtime_error("Empty configuration file");
    return config_content;
}


void    Config::insertPort(std::string value) {
    std::istringstream portStream(value);
    std::string portValue;
    int port;

    while (std::getline(portStream, portValue, ',')) {
        if (portValue.find_first_not_of("0123456789") != std::string::npos)
            throw std::runtime_error("PORT ERROR: invalid port number");
        if (portValue.size() > 5)
            throw std::runtime_error("PORT ERROR: invalid port number");
        port = atoull(portValue);
        if (port < 1024 || port > 65535)
            throw std::runtime_error("PORT ERROR: cannot bind to this port number");
        ports.insert(port);
    }
}

void    Config::insertHost(std::string value) {
    std::istringstream hostStream(value);
    std::string Octet;
    int count = 0;
    while (std::getline(hostStream, Octet, '.')) {
        if (Octet.empty())
            throw std::runtime_error("HOST ERROR: invalid host number");
        if (Octet.find_first_not_of("0123456789") != std::string::npos)
            throw std::runtime_error("HOST ERROR: invalid host number");
        if (Octet.size() > 3)
            throw std::runtime_error("HOST ERROR: invalid host number");
        if (atoull(Octet) > 255)
            throw std::runtime_error("HOST ERROR: invalid host number");
        count++;
    }
    if (count != 4)
        throw std::runtime_error("HOST ERROR: invalid host number");
    host = value;
}

void    Config::insertAllowedMethods(std::string value) {
    std::istringstream methodStream(value);
    std::string method;

    while (std::getline(methodStream, method, ',')) {
        if (method.empty())
            throw std::runtime_error("METHOD ERROR: empty method, git good ^^");
        if (method != "GET" && method != "POST" && method != "DELETE")
            throw std::runtime_error("METHOD ERROR: invalid method");
        allowed_methods.insert(method);
    }
}

void    Config::insertMaxBodySize(std::string value) {
    if (value.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("MAX BODY SIZE ERROR: invalid max body size");
    try {
        max_body_size = atoull(value);
    }
    catch (std::exception &e) {
        throw std::runtime_error("MAX BODY SIZE ERROR: invalid max body size");
    }
}

void    Config::insertServerNames(std::string value) {
    std::istringstream serverStream(value);
    std::string serverName;

    while (std::getline(serverStream, serverName, ',')) {
        if (serverName.empty())
            throw std::runtime_error("SERVER NAME ERROR: empty server name");
        server_names.push_back(serverName);
    }
}

bool validErrorCode(std::string errorCode) {
    const char* errorCodes[] = {"400", "401", "402", "403", "404", "405",
                                 "406", "407", "408", "409", "410", "411",
                                "412", "413", "414", "415", "416", "417", 
                                "418", "421", "422", "423", "424", "425",
                                "426", "428", "429", "431", "451", "500",
                                "501", "502", "503", "504", "505", "506",
                                "507", "508", "510", "511"};

    for (int i = 0; i < 39; i++) {
        if (errorCode == errorCodes[i])
            return true;
    }
    return false;
}

void    Config::insertErrorPages(std::string value) {
    std::istringstream errorStream(value);
    std::string error;

    while (std::getline(errorStream, error, ',')) {
        if (error.empty())
            throw std::runtime_error("ERROR_PAGES ERROR: empty error page");
        if (error.find(':') == std::string::npos)
            throw std::runtime_error("ERROR_PAGES ERROR: invalid error page");
    
        std::string errorCode = error.substr(0, error.find(':'));
        if (!validErrorCode(errorCode))
            throw std::runtime_error("ERROR_PAGES ERROR: invalid error code");
        std::string errorPage = error.substr(error.find(':') + 1);
        if (errorPage.empty())
            throw std::runtime_error("ERROR_PAGES ERROR: empty error page");
        error_pages[atoull(errorCode)] = errorPage;
    }
}

bool    validRouteRule(std::string rule) {
    const char* validRules[] = {"ROOT", "ALLOWED_METHODS", "REDIRECT", "DEFAULT_FILE",
                                "DIR_LISTING", "MAX_BODY_SIZE", "CGI_EXTENTION", "UPLOAD_DIR"};

    for (int i = 0; i < 8; i++) {
        if (rule == validRules[i])
            return true;
    }
    return false;
}

bool validCgiExtension(std::string& ext) {
    const char* cgi_extensions[] = { ".cgi", ".pl", ".py", ".php", ".asp",
                                    ".shtml", ".fcgi", ".sh", ".plx", ".jsp", ".rb", ".xml" };

    for (int i = 0; i < 12; i++) {
        if (ext == cgi_extensions[i])
            return true;
    }
    return false;
}


void    Config::insertRoute(std::string value) {
    Route route;
    std::string path;
    std::string routeValue;
    std::string rule;
    if (value.find(':') == std::string::npos)
        throw std::runtime_error("ROUTE ERROR: invalid route syntax");
    path = value.substr(0, value.find(':'));
    if (path.empty())
        throw std::runtime_error("ROUTE ERROR: empty route path");
    if (path[0] != '/')
        throw std::runtime_error("ROUTE ERROR: invalid route path, should start with /");
    routeValue = value.substr(value.find(':') + 1);
    if (routeValue.empty())
        throw std::runtime_error("ROUTE ERROR: empty route value");

    std::istringstream routeConfig(routeValue);

    while (std::getline(routeConfig, rule, ',')) {
        if (rule.empty())
            throw std::runtime_error("ROUTE ERROR: empty route rule");
        if (rule.find('=') == std::string::npos)
            throw std::runtime_error("ROUTE ERROR: invalid route rule, missing =");
        std::string key = rule.substr(0, rule.find('='));
        if (!validRouteRule(key))
            throw std::runtime_error("ROUTE ERROR: invalid route rule");
        std::string value = rule.substr(rule.find('=') + 1);
        if (value.empty())
            throw std::runtime_error("ROUTE ERROR: empty route rule value");
        if (key == "ROOT") {
            route.root = rule.substr(rule.find('=') + 1);
        }
        else if (key == "ALLOWED_METHODS") {
            std::istringstream methodStream(value);
            std::string method;
            while (std::getline(methodStream, method, '-')) {
                if (method.empty())
                    throw std::runtime_error("ROUTE ERROR: empty method");
                if (method != "GET" && method != "POST" && method != "DELETE")
                    throw std::runtime_error("ROUTE ERROR: invalid method");
                route.allowed_methods.insert(method);
            }
        }
        else if (key == "REDIRECT") { route.redirect = value; }
        else if (key == "DEFAULT_FILE") { route.default_file = value; }
        else if (key == "DIR_LISTING") {
            if (value == "on" || value == "1")
                route.dir_listing = true;
            else if (value == "off" || value == "0")
                route.dir_listing = false;
            else
                throw std::runtime_error("ROUTE ERROR: invalid dir listing value");
        }
        else if (key == "MAX_BODY_SIZE") {
            if (value.find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("ROUTE ERROR: invalid max body size");
            try {
                route.max_body_size = atoull(value);
            }
            catch (std::exception &e) {
                throw std::runtime_error("ROUTE ERROR: invalid max body size");
            }
        }
        else if (key == "CGI_EXTENTION") {
            std::istringstream cgiStream(value);
            std::string cgi;
            while (std::getline(cgiStream, cgi, '-')) {
                if (cgi.empty())
                    throw std::runtime_error("ROUTE ERROR: empty cgi extention");
                if (!validCgiExtension(cgi))
                    throw std::runtime_error("ROUTE ERROR: invalid cgi extention");
                route.cgi_extensions.push_back(cgi);
            }
        }
        else if (key == "UPLOAD_DIR") {
            route.upload_dir = value;
        }        
    }
    routes[path] = route;
}

void    Config::printConfig() {
    std::cerr << "SERVER START [" << std::endl;
    std::cerr << "PORTS: ";
    for (std::set<int>::iterator it = ports.begin(); it != ports.end(); it++)
        std::cerr << *it << " ";
    std::cerr << std::endl;
    std::cerr << "HOST: " << host << std::endl;
    std::cerr << "ALLOWED METHODS: ";
    for (std::set<std::string>::iterator it = allowed_methods.begin(); it != allowed_methods.end(); it++)
        std::cerr << *it << " ";
    std::cerr << std::endl;
    std::cerr << "MAX BODY SIZE: " << max_body_size << std::endl;
    std::cerr << "SERVER NAMES: ";
    for (std::vector<std::string>::iterator it = server_names.begin(); it != server_names.end(); it++)
        std::cerr << *it << " ";
    std::cerr << std::endl;
    std::cerr << "ERROR PAGES: ";
    for (std::map<int, std::string>::iterator it = error_pages.begin(); it != error_pages.end(); it++)
        std::cerr << it->first << " " << it->second << " ";
    std::cerr << std::endl;
    std::cerr << "ROUTES:" << std::endl;
    for (std::map<std::string, Route>::iterator it = routes.begin(); it != routes.end(); it++) {
        std::cerr << "ROUTE: [" << it->first << "]\n{" << std::endl;
        std::cerr << "  ROOT: " << it->second.root << std::endl;
        std::cerr << "  ALLOWED METHODS: ";
        for (std::set<std::string>::iterator it2 = it->second.allowed_methods.begin(); it2 != it->second.allowed_methods.end(); it2++)
            std::cerr << *it2 << " ";
        std::cerr << std::endl;
        std::cerr << "  REDIRECT: " << it->second.redirect << std::endl;
        std::cerr << "  DEFAULT FILE: " << it->second.default_file << std::endl;
        std::cerr << "  DIR LISTING: " << it->second.dir_listing << std::endl;
        std::cerr << "  MAX BODY SIZE: " << it->second.max_body_size << std::endl;
        std::cerr << "  CGI EXTENSIONS: ";
        for (std::vector<std::string>::iterator it2 = it->second.cgi_extensions.begin(); it2 != it->second.cgi_extensions.end(); it2++)
            std::cerr << *it2 << " ";
        std::cerr << std::endl;
        std::cerr << "  UPLOAD DIR: " << it->second.upload_dir << std::endl;
        std::cerr << "}" << std::endl;
    }
    std::cerr << "] SERVER END"<< std::endl << std::endl;
}


const std::string& Config::getHost( void )  const
{
    return (host);
}

const std::set<int>& Config::getPorts( void ) const
{
    return (ports);
}

