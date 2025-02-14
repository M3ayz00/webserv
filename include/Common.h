#pragma once

#include <string>
#include <string.h>
#include <utility>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdlib>
#include <cstdio>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <errno.h>

#include <sstream>
#include <iostream>

#include <algorithm>
#include <map>
#include <vector>
#include <queue>
#include <unordered_map>

#include <cstdlib>
#include <csignal>

#define RESET "\033[0m"
#define ERROR "\033[31m"
#define INFO "\033[36m" 
#define DEBUG "\033[32m"  

std::string& strTrim(std::string& str);
std::string&  toLowerCase(std::string& str);
uint32_t stringToIpBinary(std::string addressIp);
std::string ipBinaryToString(uint32_t ipAddress);
int hexToValue(char c);
bool isHexDigit(char c);
int _16_to_10(std::string str);
std::string    timeStamp();
