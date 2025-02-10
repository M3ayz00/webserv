#pragma once

#include "Common.h"
#include "Config.hpp"

class HttpIncompleteRequest : public std::exception
{
    virtual const char * what() const throw() {return "Need more data to complete request";}
};

class HttpRequestError : public std::exception 
{
    private:
        std::string _msg;
    public:
        HttpRequestError(const std::string& msg) : _msg(msg) {}
        virtual const char* what() const throw() { return _msg.c_str(); }
};

class HttpRequest 
{
    private:
        const char* _buffer;
        size_t _pos, _bufferLen, bodyStart;
        Config  configs;

        std::string method, uri, uriPath, version, body, request;
        std::map<std::string, std::string> uriQueryParams;
        std::unordered_map<std::string, std::string> headers;


        //request-line parsing
        size_t    parseRequestLine();
        void    validateMethod();
        void    validateVersion();

        //URI parsing
        std::pair<std::string, std::string> splitKeyValue(const std::string& uri, char delim);
        bool    isAbsoluteURI();
        bool    isURIchar(char c);
        std::string decodeAndNormalize();
        std::string decode(std::string& encoded);
        std::string normalize(std::string& decoded);
        std::map<std::string, std::string> decodeAndParseQuery(std::string& query);
        bool    validateValue(std::string& value);
        
        //headers parsing
        size_t    parseHeaders();
        size_t    parseBody();
        size_t    parseChunkedBody();
        
        std::string readLine();

    public:
        enum parsingState
        {
            REQUESTLINE,
            HEADERS,
            BODY,
            COMPLETE
        };
        parsingState state;
        void    validateURI();
        HttpRequest();
        HttpRequest(const Config& _configs);
        HttpRequest(const std::string &request);
        ~HttpRequest();

        std::string& getMethod() { return method; }
        std::string& getURI() { return uri; }
        std::string& getVersion() { return version; }
        std::string& getBody() { return body; }
        std::unordered_map<std::string, std::string>& getHeaders() { return headers; }
        std::string& getUriPath() { return uriPath; }
        std::map<std::string, std::string>& getUriQueryParams() { return uriQueryParams; }
        parsingState& getState() { return state; }
        void    setURI(const std::string& _uri) { uri = _uri; }
        bool    isRequestComplete();
        
        void  setBodyStartPos(size_t value) { bodyStart = value; }
        std::string& getRequestBuffer() { return request; }
        //main parsing
        size_t    parse(const char* buffer, size_t bufferLen);
        void    clear();

};
