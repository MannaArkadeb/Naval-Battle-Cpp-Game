CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -O2 -Iinclude
LDFLAGS := -pthread

SERVER_SRC := server.cpp
CLIENT_SRC := client.cpp

.PHONY: all clean

all: server client

server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $@ $(LDFLAGS)

client: $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $@ $(LDFLAGS)

clean:
	rm -f server client