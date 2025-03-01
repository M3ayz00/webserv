#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


class HttpResponse
{
#define READ_BUFFER_SIZE 1024000

private:
    
    static void generateStatusCodes();
    static void generateMimeTypes();   
public:
    Config     serverConfig;
    long        statusCode; // pair <code, msg>
    std::string responseHeaders;
    std::string responseBody; // for error code pages now
    std::string requestedContent;

    std::string contentType;
    long      contentLength;
    std::string Date;
    std::string Server;
    std::string Connection;
    std::string extension;
    static std::map<int, std::string> statusCodesMap;
    static std::map<std::string, std::string> mimeTypes;

    void           generateAutoIndex(std::string& path, HttpRequest& request);
    std::string    generateErrorPage(size_t code);
    void        setErrorPage(std::map<int, std::string>& ErrPages);
    void    generateResponse(HttpRequest& request);
    void    prepareHeaders(std::string& path);
    void    setResponseStatusCode(unsigned code) { statusCode = code; }
    
    bool    isCgiScript(HttpRequest& request);
    void    handleCgiScript(HttpRequest &request);
    void    GET(HttpRequest& request);
    void    POST(HttpRequest& request);
    void    DELETE(HttpRequest& request);
    HttpResponse(Config& conf);
    void    reset()
    {
        requestedContent.clear();
        statusCode = 200;
        contentLength = 0;
        contentType = "text/html";
        responseHeaders.clear();
        responseBody.clear();
        Date.clear();
        Connection.clear();
    }
    std::string    combineHeaders();
};

unsigned    checkFilePerms(std::string& path);