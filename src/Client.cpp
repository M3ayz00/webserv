#include "../include/Client.hpp"


Client::Client(int client_fd, Config& Conf)
: client_fd(client_fd), client_config(Conf), request(Conf), response(Conf), sendOffset(0), state(READING_REQUEST), keepAlive(1)
{}

