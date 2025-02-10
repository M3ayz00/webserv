#include "../../include/HttpRequest.hpp"
#include <algorithm>
#include <stdexcept>

HttpRequest::HttpRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string requestLine;
    std::getline(iss, requestLine);
    if (requestLine.empty() || requestLine[requestLine.size() - 1] != '\r')
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
    std::istringstream oss(requestLine);
    oss >> method >> uri >> version;
    if ((method.empty() || uri.empty() || version.empty()))
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
    std::cout << "LOG: received request:\n      \"" << method << "\" " <<  "\"" << uri << "\" " <<  "\"" << version << "\" " << std::endl; 
    if ((method == "GET" || method == "POST" || method == "DELETE")
        && (version == "HTTP/1.0" || version == "HTTP/1.1"))
    {
        while (std::getline(iss, requestLine)) {
            if (requestLine == "\r" || requestLine.empty()) break;
            std::size_t pos = requestLine.find(':');
            std::cout << "      " << requestLine << std::endl;
            if (pos == std::string::npos)
                throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 25\r\n\r\nMalformed request header");
            std::string key = requestLine.substr(0, pos);
            std::string value = requestLine.substr(pos + 1);
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));        
            if (value.empty())
                throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 19\r\n\r\nEmpty header value");
            headers[toLowerCase(key)] = value;
        }
        if (method == "POST")
        {
            // size_t contentLen = stoi(headers["content-length"]);
            std::string body;
            while (std::getline(iss, requestLine))
                body += requestLine.substr(0, requestLine.find_last_not_of("\r") + 1);
            std::cout << "==>BODY : \n" << body << "\n_________________\n" << std::endl;
            // if (body.size() == contentLen)
            this->body = body;
            // throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
        }
    }
    else
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
}


HttpRequest::HttpRequest() : state(REQUESTLINE), _pos(0), _bufferLen(0) {}

HttpRequest::~HttpRequest() {}

HttpRequest::HttpRequest(const Config& _configs) : state(REQUESTLINE), _pos(0), _bufferLen(0), configs(_configs) {}

/*
for i in {1..100}; do
  curl -v GET 127.0.0.1:4383
done


for i in {1..100}; do
  curl -v GET 127.0.0.1:4383 &
done
wait

for i in {1..100}; do
  curl -v -X POST -d "param1=value1&param2=value2" 127.0.0.1:4383 &
done
wait
*/


size_t    HttpRequest::parse(const char* buffer, size_t bufferLen)
{
    this->_bufferLen = bufferLen;
    this->_buffer = buffer;
    size_t bytesReceived = 0;

    try
    {
        if (state == REQUESTLINE)
            bytesReceived += parseRequestLine(); 
        if (state == HEADERS) 
            bytesReceived += parseHeaders();
        if (state == BODY) 
            bytesReceived += parseBody(); 
        if (state == COMPLETE) 
        {
            std::cout << DEBUG << "FULL PARSED REQUEST : \n"
                << "          -method: " << method 
                <<"\n          -uri: " << uri
                <<"\n          -version: " << version
                <<"\n          -headers:\n";
            for (auto header : headers)
                std::cout << "              -" << header.first << ": " << header.second << "\n";
            std::cout << "          -body: \n{" << body << "\n}\n" << RESET;  
            return 0;
        }
    }
    catch(const HttpIncompleteRequest& e)
    {
        return (bytesReceived);
    }
    return (bytesReceived);
}

// METHOD SP URI SP VERSION CRLF
size_t    HttpRequest::parseRequestLine()
{
    std::string line = readLine();
    if (line.empty())
        throw (HttpIncompleteRequest());
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace == std::string::npos || secondSpace == std::string::npos)
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 22\r\n\r\nMalformed request line")); 
    if (line.find(' ', secondSpace + 1) != std::string::npos)
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 22\r\n\r\nMalformed request line")); 
    method = line.substr(0, firstSpace);
    uri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = line.substr(secondSpace + 1);
    validateMethod();
    validateURI();
    validateVersion();
    // std::cout << DEBUG << "DEBUG: " << method << " " << uri << " " << version << "\n"; 
    state = HEADERS;
    return (line.size() + 2);
}

void    HttpRequest::validateMethod()
{
    if (method != "GET" && method != "DELETE" && method != "POST")
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nMalformed request line"));
}

void    HttpRequest::validateURI()
{
    if (uri.empty())
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nBad URI"));
    std::pair<std::string, std::string> pathAndQuery = splitKeyValue(uri, '?');
    uriPath = pathAndQuery.first;
    std::string query = pathAndQuery.second;
    if (!isAbsoluteURI())
    {
        if (uriPath[0] != '/')
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nBad URI"));
    }
    else
    {
        size_t schemeEnd = uriPath.find('/');
        if (schemeEnd != std::string::npos)
            uriPath = uriPath.substr(schemeEnd + 2);
        else
            uriPath = "/";
    }
    uriPath = decodeAndNormalize();
    if (!query.empty())
        uriQueryParams = decodeAndParseQuery(query);
}

std::map<std::string, std::string> HttpRequest::decodeAndParseQuery(std::string& query)
{
    std::istringstream iss(query);
    std::map<std::string, std::string> queryParams;
    std::string queryParam, key, value;
    while (std::getline(iss, queryParam, '&'))
    {
        size_t pos = queryParam.find('=');
        key = queryParam.substr(0, pos);
        value = (pos != std::string::npos ? queryParam.substr(pos + 1) : "");
        if (value.find('#') != std::string::npos)
            value.erase(value.find('#'));
        queryParams[key] = value;//decode this
    }
    return (queryParams);
}

std::string HttpRequest::decode(std::string& encoded)
{
    std::string decoded;
    for (size_t i = 0; i < encoded.size(); i++)
    {
        if (encoded[i] == '%')
        {
            if (i + 2 >= encoded.size())
                throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nInvalid percent-encoding"));
            if (!isHexDigit(encoded[i + 1]) || !isHexDigit(encoded[i + 1]))                
                throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nInvalid percent-encoding"));
            decoded += hexToValue(encoded[i + 1]) * 16 + hexToValue(encoded[i + 2]);
            i += 2;
        }
        else if (!isURIchar(encoded[i]))
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInvalid URI character"));
        else
            decoded += encoded[i];
    }
    return (decoded);
}

std::string HttpRequest::normalize(std::string& decoded)
{
    std::istringstream iss(decoded);
    std::string segment;
    std::vector<std::string> normalizedSegments;
    std::string normalized;
    while (std::getline(iss, segment, '/'))
    {
        if (segment == ".") continue ;
        else if (segment == "..")
        {
            if (!normalizedSegments.empty()) 
                normalizedSegments.pop_back();
        }
        else 
            normalizedSegments.push_back(segment);
    }
    for (size_t i = 0; i < normalizedSegments.size(); i++)
    {
        if (normalizedSegments[i].empty()) continue;
        normalized += "/" + normalizedSegments[i];
    }
    if (normalized.empty())
        return ("/");
    return (normalized);
}

std::string HttpRequest::decodeAndNormalize()
{
    std::istringstream iss(uriPath);
    std::string segment;
    std::string decodedAndNormalized;
    while (std::getline(iss, segment, '/'))
        decodedAndNormalized += "/" + decode(segment);
    return (normalize(decodedAndNormalized));
}

bool    HttpRequest::isAbsoluteURI()
{
    return (uri.find("http://") == 0);
}

bool    HttpRequest::isURIchar(char c)
{
    const std::string allowed = "!$&'()*+,-./:;=@_~";
    return (std::isalnum(c) || allowed.find(c) != std::string::npos);
}

void    HttpRequest::validateVersion()
{
    if (version.empty() || version != "HTTP/1.1")
        throw (HttpRequestError("HTTP/1.1 505 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nVersion not supported"));
}

//header-field   = field-name ":" OWS field-value OWS
size_t    HttpRequest::parseHeaders()
{
    size_t startPos = _pos;
    std::string line = readLine();
    while (!line.empty())
    {
        std::pair<std::string, std::string> keyValue = splitKeyValue(line, ':');
        std::string key = toLowerCase(keyValue.first);
        std::string value = strTrim(keyValue.second);
        if (key.empty() || key.find_first_of(" \t") != std::string::npos || key.find_last_of(" \t") != std::string::npos)
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nMalformed header field"));
        if (headers.find(key) != headers.end())
            headers[key] += "," + value;
        else
            headers[key] = value;
        line = readLine();
    }
    state = BODY;
    if (!headers.count("host"))
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nMalformed header field"));
    bodyStart = _pos;
    return (_pos - startPos);
}

//curl -H "Tranfert-Encoding: chunked" -F "filename=/path" 127.0.0.1:8080 (for testing POST)
size_t    HttpRequest::parseBody()
{
    size_t startPos = _pos;
    if (headers.count("transfer-encoding") && toLowerCase(headers["transfer-encoding"]).find("chunked") != std::string::npos)
    {
        if (headers.count("content-length"))
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 33\r\n\r\nConflicting Transfer-Encoding and Content-Length"));
        return (parseChunkedBody());
    }
    else if (headers.count("content-length"))
    {
        int contentLength = std::atoi(headers["content-length"].c_str());
        if (_bufferLen - _pos < contentLength)
            throw (HttpIncompleteRequest());
        body.append(_buffer + _pos, contentLength);
        _pos += contentLength;
    }
    state = COMPLETE;
    return (_pos - startPos);
}

size_t    HttpRequest::parseChunkedBody()
{
    size_t startPos = _pos;
    while (true)
    {
        std::string line = readLine();
        int chunkSize = _16_to_10(line);
        if (chunkSize < 0)
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nMalformed header field"));
        else if (chunkSize == 0)
        {
            readLine();
            break ;
        }
        if (_bufferLen - _pos < chunkSize + 2)
            throw (HttpIncompleteRequest());
        body.append(_buffer + _pos, chunkSize);
        _pos += chunkSize + 2;
    }
    state = COMPLETE;
    return (_pos - startPos);
}

std::string HttpRequest::readLine()
{
    size_t start = _pos;
    while (_pos < _bufferLen)
    {
        if (_buffer[_pos] == '\r' && _pos + 1 < _bufferLen && _buffer[_pos + 1] == '\n')
        {
            std::string line(_buffer + start, _pos - start);
            _pos += 2;
            return (line);
        }
        _pos++;
    }
    _pos = start;
    throw (HttpIncompleteRequest());
}

std::pair<std::string, std::string> HttpRequest::splitKeyValue(const std::string& toSplit, char delim)
{
    size_t keyValue = toSplit.find(delim);
    std::string key = toSplit.substr(0, keyValue);
    std::string value = (keyValue != std::string::npos ? toSplit.substr(keyValue + 1) : "");
    return (std::make_pair(key, value));
}

bool    HttpRequest::isRequestComplete()
{
    return (state == COMPLETE);
}

void    HttpRequest::clear()
{
    this->body.clear();
    this->bodyStart = 0;
    this->state = REQUESTLINE;
    this->method.clear();
    this->uri.clear();
    this->uriPath.clear();
    this->request.clear();
    this->version.clear();
    this->_pos = 0;
    this->_bufferLen = 0;
    this->_buffer = NULL;
    this->headers.clear();
}