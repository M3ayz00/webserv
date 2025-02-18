#include "../../include/Server.hpp"
#include "../../include/HttpRequest.hpp"

Server::Server(const Config& serverConfig)
{
    // give this func another name
    this->serverConfig = serverConfig;
    std::set<int>::const_iterator it = serverConfig.ports.begin();
    while (it != serverConfig.ports.end())
    {
        Socket* serverSocket;
        try
        {
            serverSocket = new Socket;
            serverSocket->create();
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(*it);
            serverAddr.sin_addr.s_addr = htonl(stringToIpBinary(serverConfig.host));
            serverSocket->bind(serverAddr);
            serverSocket->listen(SOMAXCONN);
            listeningSockets.push_back(serverSocket);
        }
        catch(const std::exception& e)
        {
            delete serverSocket;
            std::cerr << ERROR << timeStamp() << "ERROR:  setting up server on port " << *it << ": " << e.what()  << RESET << std::endl;
        }
        ++it;
    }
}

int Server::acceptConnection(int listeningSocket)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int clientSocket = accept(listeningSocket, (struct sockaddr*)&client_addr, &client_len);
    if (clientSocket == -1)
        throw std::runtime_error(ERROR + timeStamp() + "ERROR:  accepting connection: " + std::string(strerror(errno)) + std::string(RESET));
    std::clog << INFO << timeStamp() << "INFO: New client connected: [" << ipBinaryToString(client_addr.sin_addr.s_addr) << "].\n" << RESET;
    return (clientSocket);
}

void Server::shutdownServer()
{
    for (std::vector<Socket*>::iterator socket = listeningSockets.begin(); 
        socket != listeningSockets.end();
        socket++)
        delete *socket;
    std::clog << INFO << timeStamp() << "INFO: Server shut down.\n" << RESET ;
}


Server::~Server()
{
    std::cerr << INFO << timeStamp() << "INFO: Server shutting down...\n" << RESET ;
    shutdownServer();
}

const std::vector<Socket*>& Server::getListeningSockets( void ) const
{
    return (listeningSockets);
}
