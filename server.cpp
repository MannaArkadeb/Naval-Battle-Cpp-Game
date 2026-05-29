#include <cstdlib>
#include <iostream>
#include "BattleshipServer.h"

int main(int argc, char** argv) {
    const int port = (argc >= 2) ? std::atoi(argv[1]) : 4000;
    const std::size_t playersPerMatch = (argc >= 3) ? static_cast<std::size_t>(std::max(2, std::atoi(argv[2]))) : 2;

    BattleshipServer server(port, playersPerMatch);
    if (!server.start()) {
        return 1;
    }

    std::cout << "Press Enter to stop the server.\n";
    std::string line;
    std::getline(std::cin, line);

    server.stop();
    return 0;
}