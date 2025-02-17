#include "../../include/ServerManager.hpp"
#include "../../include/HttpResponse.hpp"

static ServerManager *g_manager = NULL;

ServerManager::ServerManager() : epollFd(-1), events() {}

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
        Clients.erase(fd);
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


void ServerManager::sendResponse(int clientSocket) {
    std::map<int, Client>::iterator It = Clients.find(clientSocket);
    if (It == Clients.end()) return;
    
    Client& client = It->second;
    HttpResponse& response = client.getResponse();
    
    switch (client.getClientState())
    {
        case GENERATING_RESPONSE: {
            client.setKeepAlive(client.getRequest().getHeaderValue("Connection") != "close");
            client.sendBuffer = response.responseHeaders;
            response.responseHeaders.clear();
            client.setState(SENDING_HEADERS);
        }
        case SENDING_HEADERS: {
            if (response.statusCode >= 400) {
                client.sendBuffer += response.responseBody;
            }
            ssize_t bytesSent = send(clientSocket,client.sendBuffer.c_str() + client.sendOffset,client.sendBuffer.size() - client.sendOffset, 0);
            if (bytesSent < 0)
                return closeConnection(clientSocket);
            client.sendOffset += bytesSent;
            if (client.sendOffset >= client.sendBuffer.size()) {
                client.sendBuffer.clear();
                client.sendOffset = 0;
                client.setState(SENDING_BODY);
            }
            if (response.statusCode >= 400 || response.statusCode == 301) {
                client.setState(COMPLETED);
                goto complete;
            }
        }
        break;
        case SENDING_BODY: {
            if (client.sendBuffer.empty()) {
                if (!client.file.is_open()) {
                    client.file.open(client.getRequest().getUriPath().c_str(), std::ios::binary);
                    if (!client.file.is_open()) {
                        client.setState(COMPLETED);
                        break;
                    }
                }

                char buffer[8192];
                client.file.read(buffer, 8192);
                std::streamsize bytesRead = client.file.gcount();
                
                if (bytesRead > 0) {
                    client.sendBuffer.append(buffer, bytesRead);
                } else {
                    client.setState(COMPLETED);
                }
            }
            ssize_t bytesSent = send(clientSocket, client.sendBuffer.c_str() + client.sendOffset,
                                                    client.sendBuffer.size() - client.sendOffset, 0);
            if (bytesSent < 0) {
                return closeConnection(clientSocket);
            }
            client.sendOffset += bytesSent;
            if (client.sendOffset >= client.sendBuffer.size()) {
                client.sendBuffer.clear();
                client.sendOffset = 0;
            }
        }
        break;
        complete:
        case COMPLETED: {
            client.file.close();
            client.getRequest().reset();
            client.getResponse().reset();
            
            if (response.statusCode == 301) {
                client.setState(READING_REQUEST);
                closeConnection(clientSocket);
                return ;
            }
            if (client.shouldKeepAlive()) {
                client.setState(READING_REQUEST);
                modifyEpollEvent(clientSocket, EPOLLIN);
            } else {
                closeConnection(clientSocket);
            }
        }
        break;
        default:
            break;
    }
}


void    ServerManager::handleConnections(int listeningSocket)
{
    try {
        Server* server = findServerBySocket(listeningSocket);
        int clientFD = server->acceptConnection(listeningSocket);
        if (clientFD == -1) return ;
        addToEpoll(clientFD);        
        Clients.insert(std::make_pair(clientFD, Client(clientFD, server->getserverConfig())));
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


void ServerManager::readRequest(Client& Client) {
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    int bytesReceived;
    HttpRequest& request = Client.getRequest();

    bytesReceived = recv(Client.getFd(), buffer, bufferSize, 0);
    if (bytesReceived == -1) {
        std::clog << ERROR << timeStamp() << "ERROR: receiving data in client socket N" << Client.getFd() << "\n" << RESET;
        return closeConnection(Client.getFd());
    }
    else if (bytesReceived == 0) {
        return closeConnection(Client.getFd());
    }

    std::string& requestBuffer = request.getRequestBuffer();
    requestBuffer.append(buffer, bytesReceived);
    bytesReceived = request.parse(requestBuffer.c_str(), requestBuffer.size());
    if (bytesReceived > 0)
        requestBuffer.erase(0, bytesReceived);
}

// here where u should parse the request
void ServerManager::handleRequest(int clientSocket)
{
    std::map<int, Client>::iterator It = Clients.find(clientSocket);
    if (It == Clients.end()) return ;
    
    Client& client = It->second;
    HttpRequest& request = client.getRequest();
    HttpResponse& response = client.getResponse();
    if (client.getClientState() == READING_REQUEST) {
        readRequest(client);
        
        if (request.getState() == COMPLETE) {
            client.setState(GENERATING_RESPONSE);
            response.generateResponse(request);
            modifyEpollEvent(clientSocket, EPOLLOUT);
        }
    }
}


void ServerManager::handleEvent(const epoll_event& event) {
    int fd = event.data.fd;
    if (event.events & EPOLLIN) { // ready to recv
        std::cout << "ready to READ data\n";
        if (isListeningSocket(fd)) {
            handleConnections(fd);
        }else{
            handleRequest(fd);
        }
    }
    if (event.events & EPOLLOUT) { 
        std::cout << "ready to write data\n";
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