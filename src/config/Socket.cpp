#include "../../include/Socket.hpp"

Socket::Socket() : fd(-1) {}

void  Socket::create()
{
  fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd == -1)
      throw std::runtime_error("ERROR:  creating socket: " + std::string(strerror(errno)));
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &opt, sizeof(opt)) == -1)
    throw std::runtime_error("ERROR:  setting socket opt: " + std::string(strerror(errno)));
}

void  Socket::listen(int backlog)
{
  if (::listen(fd, backlog))
    throw std::runtime_error("Listening failed: " + std::string(strerror(errno)));
}

void  Socket::bind(const sockaddr_in& server_addr)
{
  if (::bind(fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    throw std::runtime_error("Binding failed: " + std::string(strerror(errno)));
}

int  Socket::accept()
{
  sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd == -1)
    throw std::runtime_error("ERROR:  accepting connection: " + std::string(strerror(errno)));
  return (client_fd);
}

int Socket::getFd( void ) const
{
  return (fd);
}

Socket::~Socket()
{
  if (fd != -1)
  {
    close(fd);
    std::cerr << DEBUG << timeStamp() << "DEBUG: closing file descriptor " << fd << '\n' << RESET;
  }
}