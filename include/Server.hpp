#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"
#include "Socket.hpp"

class Server
{
    private:
        std::vector<Socket*> listeningSockets;
        Config serverConfig;

        void shutdownServer();

    public:
        Server(const Config& serverConfig);
        ~Server();

        int acceptConnection(int listeningSocket);

        Config& getserverConfig() { return serverConfig; }
        const std::vector<Socket*>& getListeningSockets( void ) const;
};
