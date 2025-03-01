#pragma once

#include "Common.h"
#include "Config.hpp"

class HttpIncompleteRequest : public std::exception
{
    virtual const char * what() const throw() {return "Need more data to complete request";}
};

enum parsingState
{
    REQUESTLINE,
    HEADERS,
    BODY,
    COMPLETE
};

class HttpRequest 
{
    private:
        parsingState state;
        const uint8_t* _buffer;
        size_t _pos, _bufferLen, bodyStart;
        Config  configs;
        std::string defaultIndex;
        unsigned    statusCode;
        std::string originalUri;
        bool        autoIndex;
        int     _currentChunkSize;
        unsigned long long _currentChunkBytesRead;  
        bool    fileCreated;
        bool    isChunked;
        unsigned long long _totalBodysize;
        std::string outfilename;
        std::string method, uri, uriPath, version;
        std::vector<uint8_t> body;
        std::vector<uint8_t> request;
        std::map<std::string, std::string> uriQueryParams;
        std::map<std::string, std::string> headers;
        std::string  RequestrouteKey;
        Route   routeConf;
        //request-line parsing
        size_t    parseRequestLine();
        void    validateMethod();
        void    validateVersion();
        void    RouteURI();
        bool    keepAlive;
        //URI parsing
        std::pair<std::string, std::string> splitKeyValue(const std::string& uri, char delim);
        bool    isAbsoluteURI();
        bool    isURIchar(char c);
        std::string decodeAndNormalize();
        std::string decode(std::string& encoded);
        std::string normalize(std::string& decoded);
        std::map<std::string, std::string> decodeAndParseQuery(std::string& query);
        
        //headers parsing
        size_t    parseHeaders();
        size_t    parseBody();
        size_t    parseChunkedBody();
        
        std::vector<uint8_t> readLine();

    public:

        void    validateURI();
        HttpRequest();
        HttpRequest(const Config& _configs);
        HttpRequest(const std::string &request);
        ~HttpRequest();


        std::string getRequestrouteKey();
        void    setStatusCode(long code) { statusCode = code; }
        std::string getDefaultIndex() { return defaultIndex; }
        long  getStatusCode() { return statusCode; }
        std::string getOriginalUri() { return originalUri; }

        std::string     getHeaderValue(std::string key) {
            return headers.find(key) != headers.end() ? headers[key] : "nah"; 
        }
        std::string& getMethod() { return method; }
        std::string& getURI() { return uri; }
        std::string& getVersion() { return version; }
        std::vector<uint8_t>& getBody() { return body; }
        std::map<std::string, std::string>& getHeaders() { return headers; }
        std::string& getUriPath() { return uriPath; }
        std::map<std::string, std::string>& getUriQueryParams() { return uriQueryParams; }
        parsingState& getState() { return state; }
        Config& getConfig() { return configs; }
        bool     getautoIndex () {return autoIndex; }
        Route&  getRouteConf() {return routeConf; }
        void    setURI(const std::string& _uri) { uri = _uri; }
        void    setURIpath(const std::string& _uripath) { uriPath = _uripath; }
        void  setBodyStartPos(size_t value) { bodyStart = value; }
        std::vector<uint8_t>& getRequestBuffer() { return request; }
        //main parsing
        size_t    parse(const uint8_t *buffer, size_t bufferLen);
        
        std::string getLineAsString(const std::vector<uint8_t>& line) {
            return std::string(reinterpret_cast<const char*>(line.data()), line.size());
        }
        bool    isRequestComplete();
        
        std::string getUploadDir() {
            Route& routeConf = getConfig().getRoutes()[RequestrouteKey];
            if (!routeConf.upload_dir.empty()) {
                return std::string(routeConf.upload_dir);
            }
            return "www/html/uploads/";
        }

        void    reset() {
            _buffer = 0;
            _bufferLen = 0;
            bodyStart = 0;
            defaultIndex.clear();
            autoIndex = false;
            statusCode = 200;
            fileCreated = false;
            outfilename.clear();
            _currentChunkSize = -1;
            _totalBodysize = 0;
            _currentChunkBytesRead = 0;
            method.clear();
            uriPath.clear();
            version.clear();
            body.clear();
            uri.clear();
            request.clear();
            originalUri.clear();
            uriQueryParams.clear();
            headers.clear();
            RequestrouteKey.clear();
            isChunked = false;
            // routeConf = Route();
            keepAlive = true;
            _pos = 0;
            autoIndex = false;
            state = REQUESTLINE;
        }
};
