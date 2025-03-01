#include "../include/Common.h"

std::string& strTrim(std::string& str)
{
  str.erase(0, str.find_first_not_of(" \t"));
  str.erase(str.find_last_not_of(" \t") + 1);
  return (str);
}

std::string&  toLowerCase(std::string& str)
{
  for (size_t i = 0; i < str.size(); i++)
    str[i] = std::tolower(str[i]);
  return (str);
}

uint32_t stringToIpBinary(std::string addressIp)
{
  uint32_t ip[4];
  std::istringstream iss(addressIp);
  std::string octet;
  uint32_t actualIpAddress = 0;
  for (int i = 0; i < 4; i++)
  {
    std::getline(iss, octet, '.');
    ip[i] = std::atoi(octet.c_str());
    actualIpAddress |= (ip[i] << (24 - (i * 8)));
  }
  if (iss.peek() != EOF)
    throw std::invalid_argument("Invalid IP address format: " + addressIp);
  return (actualIpAddress);
}

std::string ipBinaryToString(uint32_t ipAddress)
{
  std::ostringstream oss;
  for (int i = 0; i < 4; i++)
  {
    uint32_t octet = ipAddress >> (i * 8) & 0xFF;
    oss << octet;
    if (i < 3)
      oss << ".";
  }
  return (oss.str());
}

int hexToValue(char c)
{
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c >= '0' && c <= '9') return c - '0';
  return (0);
}

int _16_to_10(std::string str)
{
  if (str.empty())
    return (-1);
  int result = 0;
  std::string trimmed = strTrim(str);
  for (size_t i = 0; i < trimmed.size(); i++)
  {
    if (!isHexDigit(trimmed[i]))
      return -1;
    result = result * 16 + hexToValue(trimmed[i]);
  }
  return (result);
}

bool isHexDigit(char c)
{
  return (std::isdigit(c) || (std::tolower(c) >= 'a' && std::tolower(c) <= 'f'));
}

std::string timeStamp()
{
    time_t timestamp = time(NULL);
    struct tm datetime = *localtime(&timestamp);
    char output[50];
    strftime(output, 50, "%D %r", &datetime);
    return ("[" + std::string(output) + "] ");
}

std::string toString(int num)
{
  std::ostringstream oss;
  oss << num;
  return (oss.str());
}

unsigned long long atoull(std::string str)
{
  if (str.empty())
    return (-1);
  unsigned long long result = 0;
  std::string trimmed = strTrim(str);
  for (size_t i = 0; i < trimmed.size(); i++)
  {
    if (!std::isdigit(trimmed[i]))
      return -1;
    result = result * 10 + trimmed[i] - '0';
  }
  return (result);
}