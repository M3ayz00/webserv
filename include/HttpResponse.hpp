#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"

class HttpResponse
{
#define READ_BUFFER_SIZE 8192

private:
    

public:
    Config     serverConfig;
    long        statusCode; // pair <code, msg>
    std::string responseHeaders;
    std::string responseBody; // for error code pages now
    std::string requestedContent;

    std::string contentType;
    long        contentLength;
    std::string Date;
    std::string Server;
    std::string Connection;


    void           generateAutoIndex(std::string& path, HttpRequest& request);
    std::string    generateErrorPage(long code);
    void        setErrorPage(std::map<int, std::string>& ErrPages);
    void    generateResponse(HttpRequest& request);
    void    prepareHeaders(std::string& path, HttpRequest& request);
    void    setResponseStatusCode(unsigned code) { statusCode = code; }
    
    
    void    GET(HttpRequest& request);
    HttpResponse(Config& conf);
    void    reset() {
        contentType.clear();
        responseHeaders.clear();
        responseBody.clear();
        Date.clear();
        Connection.clear();
    }
    std::string    combineHeaders();
};

unsigned    checkFilePerms(std::string& path);