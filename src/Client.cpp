#include "../include/Client.hpp"


Client::Client(int client_fd, Config& Conf)
: client_fd(client_fd), client_config(Conf), request(Conf), response(Conf), sendOffset(0), state(READING_REQUEST), keepAlive(1)
{}

Client::Client(const Client& C)
: client_fd(C.client_fd), client_config(C.client_config), request(C.request), response(C.response), sendOffset(C.sendOffset), state(C.state), keepAlive(C.keepAlive)
{}
