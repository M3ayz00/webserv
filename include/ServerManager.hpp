    #pragma once

    #include "Common.h"
    #include "Server.hpp"
    #include "Client.hpp"

    class ServerManager
    {
        private :
            std::vector<Config> serverPool;
            std::vector<Server*> servers;
            std::map<int, Socket*> listeningSockets;
            std::vector<struct epoll_event> events;
            std::map<int, Client> Clients;
            int epollFd;

            void	initServers();
            void    initEpoll();
            void    eventsLoop();

            void           handleEvent(const epoll_event& event);
            void           handleConnections(int listeningSocket);
            void           handleRequest(int clientSocket);
            void           readRequest(Client& Client);
            void           sendResponse(int clientSocket);

            void        processRequest(int clientSocket, const std::string& request);
            void        modifyEpollEvent(int fd, uint32_t events);
            void        sendErrorResponse(int clientSocket, const std::string& error);
            void        closeConnection(int fd);

            void    addListeningSockets(std::vector<Server*>& servers);
            bool    isListeningSocket(int fd);
            void    addToEpoll(int clientsocket);

            Server* findServerBySocket(int fd);

            void    handleRequests(int clientSocket);
            void    generateResponse(HttpRequest& request);

            void    setNonBlocking(int fd);
        
        public  :
            ServerManager();
            ServerManager(const std::vector<Config>& _serverPool);
            ~ServerManager();
    };