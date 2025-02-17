#include "../../include/HttpRequest.hpp"
#include "../../include/HttpResponse.hpp"
#include <algorithm>
#include <stdexcept>

HttpRequest::HttpRequest() : state(REQUESTLINE), _pos(0), _bufferLen(0) {}

HttpRequest::~HttpRequest() {}

HttpRequest::HttpRequest(const Config& _configs) : state(REQUESTLINE), _pos(0), _bufferLen(0), configs(_configs), statusCode(200), originalUri(), autoIndex(false) {}

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
    std::cout << "new request\n " << std::endl;
    std::cout << "buffer = " << buffer << std::endl;
    try
    {
        if (state == REQUESTLINE)
            bytesReceived += parseRequestLine(); 
        if (state == HEADERS) 
            bytesReceived += parseHeaders();
        if (state == BODY) {
            if (method == "GET")
                return state = COMPLETE, 0;
            bytesReceived += parseBody(); 
        }
        if (state == COMPLETE)
            return 0;
    }
    catch (int code)
    {
        setStatusCode(code);
        state = COMPLETE;
        return 0;
    }
    catch(const HttpIncompleteRequest& e)
    {
        return (bytesReceived);
    }
    return (bytesReceived);
}

size_t    HttpRequest::parseRequestLine()
{
    std::string line = readLine();
    if (line.empty()) throw (HttpIncompleteRequest());
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace == std::string::npos || secondSpace == std::string::npos)
        throw 400;
    if (line.find(' ', secondSpace + 1) != std::string::npos)
        throw 400;
    method = line.substr(0, firstSpace);
    uri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = line.substr(secondSpace + 1);
    validateMethod();
    validateURI();
    RouteURI();
    validateVersion();
    state = HEADERS;
    return (line.size() + 2);
}


void    HttpRequest::RouteURI()
{
    Config& conf = configs;
    std::string  routeKey;
    std::map<std::string, Route>::iterator routeIt;
    std::map<std::string, Route> routesMap = conf.getRoutes();
    
    for (routeIt = routesMap.begin(); routeIt != routesMap.end(); routeIt++) {
        if (uriPath.find(routeIt->first) == 0) {
            if (routeIt->first.size() > routeKey.size())
                routeKey = routeIt->first;
        }
    }
    if (!routeKey.empty()) {
        Route& routeConf = routesMap[routeKey];
        defaultIndex = routeConf.getDefaultFile();
        autoIndex = routeConf.getAutoIndexState();    
        if (routeKey == "/") {
            if (size_t pos = uriPath.find('/') != std::string::npos)
                uriPath.replace(pos-1, 1, routeConf.getRoot()+"/"); // back to it later
        }
        else {
            if (size_t pos = uriPath.find(routeKey) != std::string::npos) {
                uriPath.replace(pos-1, routeKey.size(), routeConf.getRoot());
            }
        }
    }
    setStatusCode(checkFilePerms(uriPath)); 
}
void    HttpRequest::validateMethod()
{
    if (method != "GET" && method != "DELETE" && method != "POST")
        throw 400;
}

void    HttpRequest::validateURI()
{
    if (uri.empty()) throw 400;
    if (uri.size() > 2048) throw 414;

    size_t queryPos = uri.find('?');
    std::string query;
    if (queryPos != std::string::npos) {
        uriPath = uri.substr(0, queryPos);    
        query = uri.substr(queryPos + 1);
    }
    else {
        uriPath = uri;
        query.clear(); 
    }
    if (!isAbsoluteURI()) {
        if (uriPath[0] != '/') throw 400;
    }
    else {
        size_t schemeEnd = uriPath.find('/');
        if (schemeEnd != std::string::npos)
            uriPath = uriPath.substr(schemeEnd + 2);
        else
            uriPath = "/";
    }
    originalUri = uriPath;
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
        queryParams[decode(key)] = decode(value);//decode this
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
                throw 400;
            if (!isHexDigit(encoded[i + 1]) || !isHexDigit(encoded[i + 1]))                
                throw 400;
            decoded += hexToValue(encoded[i + 1]) * 16 + hexToValue(encoded[i + 2]);
            i += 2;
        }
        else if (!isURIchar(encoded[i]))
            throw 400;
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
    bool hasTrailingSlash = uriPath.back() == '/';

    while (std::getline(iss, segment, '/'))
        decodedAndNormalized += "/" + decode(segment);
    std::string normalized = normalize(decodedAndNormalized); 
    if (hasTrailingSlash)
        normalized += "/";
    return (normalized);
}

bool    HttpRequest::isAbsoluteURI()
{
    return (uri.find("http://") == 0 || uri.find("https://") == 0);
}

bool    HttpRequest::isURIchar(char c)
{
    const std::string allowed = "!$&'()*+,-./:;=@_~";
    return (std::isalnum(c) || allowed.find(c) != std::string::npos);
}

void    HttpRequest::validateVersion()
{
    if (version.empty() || version != "HTTP/1.1")
        throw 505;
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
            throw 400;
        if (headers.find(key) != headers.end())
            headers[key] += "," + value;
        else
            headers[key] = value;   
        line = readLine();
    }
    state = BODY;
    if (!headers.count("host"))
        throw 400;
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
            throw 400;
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
            throw 400;
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
    if (keyValue == std::string::npos)
        throw 400;
    std::string key = toSplit.substr(0, keyValue);
    std::string value = toSplit.substr(keyValue + 1);
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