#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"
#include "Socket.hpp"

class Server
{
    private:
        sockaddr_in server_addr;
        int port;
        std::vector<int> clientSockets;
        std::vector<Socket*> listeningSockets;
        std::string host;
        Config serverConfig;

        void shutdownServer();

    public:
        Server(const Config& serverConfig);
        ~Server();

        int acceptConnection(int listeningSocket);
        void closeConnection(int client_fd);

        Config& getserverConfig() { return serverConfig; }
        const std::vector<Socket*>& getListeningSockets( void ) const;
        const std::vector<int>& getClientSockets( void ) const;
};
