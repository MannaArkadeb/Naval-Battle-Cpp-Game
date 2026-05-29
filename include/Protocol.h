#pragma once

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

#include <sys/socket.h>
#include <unistd.h>

namespace net {

inline std::string trim(std::string text) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };

    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }

    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }

    return text;
}

inline bool sendLine(int socketFd, const std::string& line) {
    std::string payload = line;
    payload.push_back('\n');

    const char* data = payload.data();
    std::size_t remaining = payload.size();

    while (remaining > 0) {
        const ssize_t sent = ::send(socketFd, data, remaining, 0);
        if (sent <= 0) {
            return false;
        }
        data += sent;
        remaining -= static_cast<std::size_t>(sent);
    }

    return true;
}

inline bool recvLine(int socketFd, std::string& line) {
    line.clear();
    char ch = 0;

    while (true) {
        const ssize_t received = ::recv(socketFd, &ch, 1, 0);
        if (received == 0) {
            return false;
        }
        if (received < 0) {
            return false;
        }

        if (ch == '\n') {
            break;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }

    return true;
}

inline bool parseJoin(const std::string& line, std::string& name) {
    std::istringstream input(trim(line));
    std::string command;
    if (!(input >> command) || command != "JOIN") {
        return false;
    }

    std::getline(input, name);
    name = trim(name);
    return !name.empty();
}

struct AttackCommand {
    bool hasTarget = false;
    int targetIndex = -1;
    int row = -1;
    int col = -1;
};

inline bool parseAttackCommand(const std::string& line, AttackCommand& command) {
    std::istringstream input(trim(line));
    std::string verb;
    if (!(input >> verb) || verb != "ATTACK") {
        return false;
    }

    int first = -1;
    int second = -1;
    int third = -1;
    if (!(input >> first >> second)) {
        return false;
    }

    if (input >> third) {
        command.hasTarget = true;
        command.targetIndex = first;
        command.row = second;
        command.col = third;
        return true;
    }

    command.hasTarget = false;
    command.targetIndex = -1;
    command.row = first;
    command.col = second;
    return true;
}

} // namespace net