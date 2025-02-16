#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"

enum ClientState {
    READING_REQUEST,
    GENERATING_RESPONSE,
    SENDING_HEADERS,
    SENDING_BODY,
    COMPLETED
};

class Client {
    public:
        int client_fd;
        std::string    sendBuffer;
        size_t         sendOffset;
        std::ifstream  file;
        size_t          fileOffset;
        HttpRequest     request;
        HttpResponse    response;
        ClientState     state;
        bool            keepAlive;
    public:

        Config& client_config; 
        Client(int client_fd, Config& Conf);
        void resetState() {
            request.reset();
            response.reset();
            if (file.is_open()) {
                file.close();
            }
            sendBuffer.clear();
            sendOffset = 0;
         }
        int getFd() const { return client_fd; }
        bool    shouldKeepAlive() {
            std::map<std::string, std::string>::iterator It = request.getHeaders().find("Connection");
            if (It != request.getHeaders().end()) {
                return It->second == "close" ? 0 : 1;
            }
            return 1; 
        }
        HttpRequest& getRequest() { return request; }
        HttpResponse& getResponse() { return response; }
        std::string& getSendBuffer() { return sendBuffer; }
        ClientState getClientState() { return state; }

        void setKeepAlive(bool keep) { keepAlive = keep; }
        void   setState(ClientState _state) { state = _state; }
};