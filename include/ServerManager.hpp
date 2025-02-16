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

        void    handleEvent(const epoll_event& event);
        void    handleConnections(int listeningSocket);
        void    handleRequest(int clientSocket);
        void    readRequest(Client& Client);
        void    sendResponse(int clientSocket);

        void    modifyEpollEvent(int fd, uint32_t events);
        void    closeConnection(int fd);

        void    addListeningSockets(std::vector<Server*>& servers);
        bool    isListeningSocket(int fd);
        void    addToEpoll(int clientsocket);

        Server* findServerBySocket(int fd);


        void    setNonBlocking(int fd);

        void    handleSignals();
        static void    handleSignal(int sig);
        void    shutDownManager();
    public  :
        ServerManager();
        ServerManager(const std::vector<Config>& _serverPool);
        ~ServerManager();
};