#include "../../include/ServerManager.hpp"

static ServerManager *g_manager = NULL;

ServerManager::ServerManager() : epollFd(-1), events(0) {}

void    ServerManager::shutDownManager()
{
    if (epollFd != -1)
    {
        close(epollFd);
        epollFd = -1;
    }
    for (size_t i = 0; i < servers.size(); i++)
        delete servers[i];
    servers.clear();
    events.clear();
    Clients.clear();
    listeningSockets.clear();
    serverPool.clear();
}

ServerManager::~ServerManager()
{
    std::clog << INFO << "INFO: Shutting down Cluster\n" << RESET;
    shutDownManager();
}

void    ServerManager::handleSignal(int sig)
{
    (void) sig;

    if (g_manager)
        g_manager->shutDownManager();
    exit(0);
}

void    ServerManager::handleSignals()
{
    g_manager = this;
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGQUIT, handleSignal);
}

bool    ServerManager::isListeningSocket(int fd)
{
    return listeningSockets.find(fd) != listeningSockets.end();
}

Server* ServerManager::findServerBySocket(int fd)
{
    if (fd < 0)
        return (NULL);
    for (int i = 0; i < servers.size(); i++)
    {
        if (isListeningSocket(fd))
        {
            const std::vector<Socket*>& listeningSockets = servers[i]->getListeningSockets();
            for (int j = 0; j < listeningSockets.size(); j++)
            {
                if (listeningSockets[j]->getFd() == fd)
                    return (servers[i]);
            }
        }
        else
        {
            const std::vector<int>& clientSockets = servers[i]->getClientSockets();     
            std::find(clientSockets.begin(), clientSockets.end(), fd);  
            for (int k = 0; k < clientSockets.size(); k++)
            {
                if (clientSockets[k] == fd)
                    return (servers[i]);
            }
        }
    }
    return (NULL);
}

void  ServerManager::setNonBlocking(int fd)
{
    if (fd < 0)
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Invalid file descriptor: " + std::to_string(fd) + std::string(RESET));
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Getting flags for client socket: " + std::string(strerror(errno)) + std::string(RESET));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) 
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Setting socket to non-blocking: " + std::string(strerror(errno)) + std::string(RESET));
}


void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::vector<Socket*> sockets = servers[i]->getListeningSockets();
        for (size_t j = 0; j < sockets.size(); j++)
        {
            int listeningSocket = sockets[j]->getFd();
            setNonBlocking(listeningSocket);
            struct epoll_event event;
            memset(&event, 0, sizeof(event));
            event.events = EPOLLIN;
            event.data.fd = listeningSocket;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listeningSocket, &event) == -1)
                throw std::runtime_error(ERROR + timeStamp() + "ERROR:  adding socket to epoll: " + std::string(strerror(errno)) + std::string(RESET));
            events.push_back(event);
        }
    }
}

void    ServerManager::addToEpoll(int clientSocket)
{
    setNonBlocking(clientSocket);
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Adding socket to epoll: " + std::string(strerror(errno)) + std::string(RESET));
}

void ServerManager::closeConnection(int fd) {
    Server* server = findServerBySocket(fd);
    if (server) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
        server->closeConnection(fd);
    }
}

void ServerManager::modifyEpollEvent(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        closeConnection(fd);
    }
}

void ServerManager::sendErrorResponse(int clientSocket, const std::string& error) {
    if (send(clientSocket, error.c_str(), error.size(), 0) == -1) {
        std::cerr << ERROR << timeStamp() << "ERROR: sending error response to client socket N" << clientSocket << "\n" << RESET;
        closeConnection(clientSocket);
    }else {
        modifyEpollEvent(clientSocket, EPOLLIN);
        Clients.at(clientSocket).getRequest().clear(); 
        std::clog << INFO << timeStamp() << "INFO: Sent an error response succesfully to client socket N" << clientSocket << "\n" << RESET;
    }
}

void ServerManager::sendResponse(int clientSocket) {
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\npain!";
    if (Clients.at(clientSocket).getRequest().isRequestComplete())
    {
        if (send(clientSocket, response.c_str(), response.size(), 0) == -1) {
            std::cerr << ERROR << timeStamp() << "ERROR: Sending data\n" << RESET;
            closeConnection(clientSocket);
        } else {
            modifyEpollEvent(clientSocket, EPOLLIN);
            Clients.at(clientSocket).getRequest().clear(); 
            std::clog << INFO << timeStamp() << "INFO: Sent a response succesfully to client socket N" << clientSocket << "\n" << RESET;
        }
    }
}


void    ServerManager::handleConnections(int listeningSocket)
{
    try {
        Server* server = findServerBySocket(listeningSocket);
        int clientFD = server->acceptConnection(listeningSocket);
        if (clientFD == -1)
            return ;
        addToEpoll(clientFD);
        Clients.insert(std::pair<int, Client>(clientFD, Client(clientFD)));
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


void ServerManager::readRequest(Client& Client) {
    // request 7atha f client.request; muhim chuf kidir hhh
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    int bytesReceived;

    bytesReceived = recv(Client.getFd(), buffer, bufferSize, 0);
    if (bytesReceived == -1) {
        std::cerr << ERROR << timeStamp() << "ERROR: receiving data in client socket N" << Client.getFd() << "\n" << RESET;
        closeConnection(Client.getFd());
        return ;
    }
    else if (bytesReceived == 0) {
        closeConnection(Client.getFd());
        return ;
    }
    Client.getRequest().getRequestBuffer().append(buffer, bytesReceived);

    std::string request;
    request = Client.getRequest().getRequestBuffer();
    bytesReceived = Client.getRequest().parse(request.c_str(), request.size());
    if (bytesReceived > 0)
        request.erase(0, bytesReceived);
    if (Client.getRequest().isRequestComplete())
    {
        modifyEpollEvent(Client.getFd(), EPOLLOUT);
        return ;
    }
}

// here where u should parse the request
void ServerManager::handleRequest(int clientSocket) {
    std::map<int, Client>::iterator Client = Clients.find(clientSocket);

    if (Client != Clients.end())
    {
        // hna l3b kima bghiti
        try {
            readRequest(Client->second);
        } catch (const std::exception& e) {
            sendErrorResponse(clientSocket, e.what());
        }
    }
    // just skip if client not found.
}


void ServerManager::handleEvent(const epoll_event& event) {
    int fd = event.data.fd;
    if (event.events & EPOLLIN) { // ready to recv
        if (isListeningSocket(fd)) {
            handleConnections(fd); // accept Client Connection
        }else{
            handleRequest(fd); // tkalef a m3ayzo
        }
    }
    else if (event.events & EPOLLOUT) { // ready to send 
        sendResponse(fd); 
    }
    if (event.events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
        std::cerr << ERROR << timeStamp() << "ERROR: " << strerror(errno) << "\n" << RESET;
        closeConnection(fd);
    }
}

void    ServerManager::eventsLoop() // events Loop (main loop)
{
    while (1) {
        int eventsNum = epoll_wait(epollFd, events.data(), events.size(), -1);
        if (eventsNum == -1){
            std::cerr << ERROR << timeStamp() << "ERROR: in epoll_wait: " << strerror(errno) << std::endl << RESET;
            continue ;
        }
        if (eventsNum == events.size())
            events.resize(events.size() * 2);

        for (int i = 0; i < eventsNum; i++){
            handleEvent(events[i]);
        }
    }
}




void  ServerManager::initServers()
{
    handleSignals();
    for (size_t i = 0; i < serverPool.size(); i++)
    {
        try {    
            Server* server = new Server(serverPool[i]);
            std::clog << INFO << timeStamp() << "INFO: Setting & starting up server :\n" << RESET << "   -host: " << serverPool[i].getHost() << "\n";
            std::set<int>::const_iterator port =  serverPool[i].getPorts().begin();
            while (port != serverPool[i].getPorts().end()) {
                std::clog << "   -port: " << *port;
                ++port;
            }
            std::clog << std::endl;
            servers.push_back(server);
            std::vector<Socket*> sockets = server->getListeningSockets();
            for (size_t j = 0; j < sockets.size(); j++)
                listeningSockets[sockets[j]->getFd()] = sockets[j];
        }
        catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
    }
}
void ServerManager::initEpoll() {
    epollFd = epoll_create1(O_CLOEXEC);
    if (epollFd == -1) {
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: creating epoll instance: " + std::string(strerror(errno) + std::string(RESET)));
    }
    addListeningSockets(servers);
}

ServerManager::ServerManager(const std::vector<Config>& _serverPool) : serverPool(_serverPool), epollFd(-1), events(0)
{
    initServers();
    initEpoll();
    eventsLoop();
}