CXX = c++
CXXFLAGS = #-Wall -Wextra -Werror #-std=c++98 

NAME = server

SRCS = src/http/HttpRequest.cpp src/config/Server.cpp src/Common.cpp src/config/Socket.cpp src/config/Config.cpp src/Route.cpp src/Webserv.cpp src/config/ServerManager.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean debug
