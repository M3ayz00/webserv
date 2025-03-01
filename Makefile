CXX = c++
CXXFLAGS = -Ofast -g3 -Wall -Wextra -Werror -std=c++98 

NAME = server

SRCS = src/http/HttpRequest.cpp src/config/Server.cpp src/Common.cpp src/config/Socket.cpp \
		src/config/Config.cpp src/Route.cpp src/Webserv.cpp src/config/ServerManager.cpp \
		src/Client.cpp src/http/HttpResponse.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS) 
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp ./include/*
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean

.SECONDARY: $(OBJS)