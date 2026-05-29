#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Protocol.h"

namespace {

int gPlayersPerMatch = 2;

bool parseCoordinateToken(const std::string& token, int& row, int& col) {
    if (token.size() < 2) {
        return false;
    }

    const char first = static_cast<char>(std::toupper(static_cast<unsigned char>(token.front())));
    const char last = static_cast<char>(std::toupper(static_cast<unsigned char>(token.back())));

    if (std::isalpha(static_cast<unsigned char>(first))) {
        const std::string number = token.substr(1);
        if (number.empty()) {
            return false;
        }
        row = first - 'A';
        col = std::atoi(number.c_str());
    } else if (std::isalpha(static_cast<unsigned char>(last))) {
        const std::string number = token.substr(0, token.size() - 1);
        if (number.empty()) {
            return false;
        }
        row = last - 'A';
        col = std::atoi(number.c_str());
    } else {
        return false;
    }

    return row >= 0 && row < 10 && col >= 0 && col < 10;
}

bool parseCoordinateTokens(const std::vector<std::string>& tokens, std::size_t startIndex, int& row, int& col) {
    if (startIndex >= tokens.size()) {
        return false;
    }

    if (parseCoordinateToken(tokens[startIndex], row, col)) {
        return true;
    }

    if (startIndex + 1 >= tokens.size()) {
        return false;
    }

    const std::string first = tokens[startIndex];
    const std::string second = tokens[startIndex + 1];

    if (first.size() == 1 && std::isalpha(static_cast<unsigned char>(first[0]))) {
        row = static_cast<char>(std::toupper(static_cast<unsigned char>(first[0]))) - 'A';
        col = std::atoi(second.c_str());
        return row >= 0 && row < 10 && col >= 0 && col < 10;
    }

    if (second.size() == 1 && std::isalpha(static_cast<unsigned char>(second[0]))) {
        row = static_cast<char>(std::toupper(static_cast<unsigned char>(second[0]))) - 'A';
        col = std::atoi(first.c_str());
        return row >= 0 && row < 10 && col >= 0 && col < 10;
    }

    return false;
}

bool buildAttackCommand(const std::string& inputLine, int playersPerMatch, std::string& commandOut) {
    std::istringstream input(net::trim(inputLine));
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return false;
    }

    std::size_t start = 0;
    if (tokens[0] == "ATTACK") {
        start = 1;
    }

    if (start >= tokens.size()) {
        return false;
    }

    int target = -1;
    if (playersPerMatch > 2) {
        if (start >= tokens.size()) {
            return false;
        }
        target = std::atoi(tokens[start].c_str());
        ++start;
    }

    int row = -1;
    int col = -1;
    if (!parseCoordinateTokens(tokens, start, row, col)) {
        return false;
    }

    if (playersPerMatch > 2) {
        commandOut = "ATTACK " + std::to_string(target) + " " + std::to_string(row) + " " + std::to_string(col);
    } else {
        commandOut = "ATTACK " + std::to_string(row) + " " + std::to_string(col);
    }
    return true;
}

bool connectToServer(int socketFd, const std::string& host, int port) {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(static_cast<uint16_t>(port));

    if (::inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) <= 0) {
        return false;
    }

    return ::connect(socketFd, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == 0;
}

void printBoard(int socketFd) {
    std::string line;
    while (net::recvLine(socketFd, line)) {
        if (line == "BOARD_END") {
            break;
        }
        std::cout << line << '\n';
    }
}

bool parseMatchStart(const std::string& line, int& players) {
    const std::string prefix = "MATCH_START players=";
    if (line.rfind(prefix, 0) != 0) {
        return false;
    }

    std::istringstream input(line.substr(prefix.size()));
    if (!(input >> players)) {
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char** argv) {
    const std::string host = (argc >= 2) ? argv[1] : "127.0.0.1";
    const int port = (argc >= 3) ? std::atoi(argv[2]) : 4000;
    std::string name = (argc >= 4) ? argv[3] : "Player";

    if (argc < 4) {
        std::cout << "Enter your name: ";
        std::getline(std::cin, name);
        if (name.empty()) {
            name = "Player";
        }
    }

    const int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    if (!connectToServer(socketFd, host, port)) {
        std::cerr << "Failed to connect to " << host << ':' << port << "\n";
        ::close(socketFd);
        return 1;
    }

    if (!net::sendLine(socketFd, "JOIN " + name)) {
        std::cerr << "Failed to send join request.\n";
        ::close(socketFd);
        return 1;
    }

    std::cout << "Connected as " << name << "\n";
    std::cout << "Waiting for the server...\n";

    std::string line;
    while (net::recvLine(socketFd, line)) {
        int players = 0;
        if (parseMatchStart(line, players)) {
            gPlayersPerMatch = players;
            std::cout << line << '\n';
            continue;
        }

        if (line == "BOARD_BEGIN OWN" || line == "BOARD_BEGIN OPPONENT") {
            std::cout << '\n' << line << '\n';
            printBoard(socketFd);
            continue;
        }

        if (line == "READY") {
            std::cout << "READY: Waiting for match start...\n";
            continue;
        }

        if (line == "YOUR_TURN") {
            if (gPlayersPerMatch > 2) {
                std::cout << "Your turn. Send ATTACK target coord, for example ATTACK 2 A5\n";
            } else {
                std::cout << "Your turn. Send ATTACK coord, for example ATTACK A5\n";
            }

            std::string command;
            while (true) {
                std::cout << "> ";
                if (!std::getline(std::cin, command)) {
                    net::sendLine(socketFd, "ATTACK 0 0");
                    break;
                }

                if (command.empty()) {
                    continue;
                }

                std::string wireCommand;
                if (buildAttackCommand(command, gPlayersPerMatch, wireCommand)) {
                    if (net::sendLine(socketFd, wireCommand)) {
                        break;
                    }
                    std::cerr << "Failed to send attack.\n";
                    break;
                }

                if (gPlayersPerMatch > 2) {
                    std::cout << "Enter ATTACK target A5 (or ATTACK target A 5).\n";
                } else {
                    std::cout << "Enter ATTACK A5 (or ATTACK A 5).\n";
                }
            }

            continue;
        }

        if (line == "HIT" || line == "MISS" || line.rfind("SUNK ", 0) == 0) {
            std::cout << line << '\n';
            continue;
        }

        if (line.rfind("OPPONENT_HIT ", 0) == 0 ||
            line.rfind("OPPONENT_MISS ", 0) == 0 ||
            line.rfind("OPPONENT_SUNK ", 0) == 0) {
            std::cout << line << '\n';
            continue;
        }

        std::cout << line << '\n';

        if (line == "WIN" || line == "LOSE" || line == "SERVER_SHUTDOWN" || line == "OPPONENT_DISCONNECTED") {
            break;
        }
    }

    ::close(socketFd);
    return 0;
}