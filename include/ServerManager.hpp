#pragma once

#include "Common.h"
#include "Server.hpp"
#include "Client.hpp"

// ANSI color codes
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define RESET   "\033[0m"

#define READ_BUFFER_SIZE 1024000 // 1MB

class ServerManager
{
    private :
    std::vector<Config> serverPool;
        int epollFd;
        std::vector<struct epoll_event> events;
        std::vector<Server*> servers;
        std::map<int, Socket*> listeningSockets;
        std::map<int, Client> Clients;

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
        void LOG(HttpRequest& request, HttpResponse& response);
        ServerManager();
        ServerManager(const std::vector<Config>& _serverPool);
        ~ServerManager();
};