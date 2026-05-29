#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

#include "Player.h"
#include "Protocol.h"

struct PendingClient {
    int socketFd = -1;
    std::string name;
};

class MatchSession {
public:
    explicit MatchSession(std::vector<PendingClient> clients) {
        m_players.reserve(clients.size());
        for (auto& client : clients) {
            const std::string name = client.name;
            m_players.push_back(Participant{std::move(client), Player(name), true});
        }
    }

    ~MatchSession() {
        shutdown();
    }

    void shutdown() {
        m_running.store(false);
        for (auto& participant : m_players) {
            closeSocket(participant.connection.socketFd);
        }
    }

    void run() {
        for (auto& participant : m_players) {
            if (!participant.player.board().placeRandomFleet(m_rng)) {
                notifyShutdown();
                shutdown();
                return;
            }
        }

        for (const auto& participant : m_players) {
            net::sendLine(participant.connection.socketFd, "READY");
        }

        broadcast("MATCH_START players=" + std::to_string(m_players.size()) + buildNameList());
        sendBoardsToAlive();

        std::size_t turnCursor = 0;
        while (m_running.load()) {
            if (aliveCount() <= 1) {
                declareWinner();
                shutdown();
                return;
            }

            const std::size_t active = nextAliveFrom(turnCursor);
            if (active == npos) {
                declareWinner();
                shutdown();
                return;
            }

            if (!net::sendLine(m_players[active].connection.socketFd, "YOUR_TURN")) {
                handleDisconnect(active);
                turnCursor = nextAliveFrom(active + 1);
                continue;
            }

            std::string commandLine;
            if (!net::recvLine(m_players[active].connection.socketFd, commandLine)) {
                handleDisconnect(active);
                turnCursor = nextAliveFrom(active + 1);
                continue;
            }

            net::AttackCommand command;
            if (!net::parseAttackCommand(commandLine, command)) {
                net::sendLine(m_players[active].connection.socketFd, "INVALID COMMAND USE ATTACK [TARGET] ROW COL");
                continue;
            }

            const std::size_t target = resolveTarget(active, command);
            if (target == npos) {
                net::sendLine(m_players[active].connection.socketFd, "INVALID_MOVE");
                continue;
            }

            const auto report = m_players[target].player.board().attack(command.row, command.col);
            if (report.outcome == Board::AttackOutcome::Invalid || report.outcome == Board::AttackOutcome::Already) {
                net::sendLine(m_players[active].connection.socketFd, "INVALID_MOVE");
                continue;
            }

            if (report.outcome == Board::AttackOutcome::Miss) {
                net::sendLine(m_players[active].connection.socketFd, "MISS");
                net::sendLine(m_players[target].connection.socketFd, "OPPONENT_MISS " + m_players[active].player.name());
            } else if (report.outcome == Board::AttackOutcome::Hit) {
                net::sendLine(m_players[active].connection.socketFd, "HIT");
                net::sendLine(m_players[target].connection.socketFd, "OPPONENT_HIT " + m_players[active].player.name());
            } else if (report.outcome == Board::AttackOutcome::Sunk) {
                std::string shipName = report.message;
                const std::string prefix = "Sunk ";
                if (shipName.rfind(prefix, 0) == 0) {
                    shipName = shipName.substr(prefix.size());
                }
                net::sendLine(m_players[active].connection.socketFd, "SUNK " + shipName);
                net::sendLine(m_players[target].connection.socketFd, "OPPONENT_SUNK " + shipName + " BY " + m_players[active].player.name());
            }
            sendBoardsToAlive();

            if (m_players[target].player.board().allSunk()) {
                eliminatePlayer(target);
            }

            if (aliveCount() <= 1) {
                declareWinner();
                shutdown();
                return;
            }

            turnCursor = nextAliveFrom(active + 1);
        }

        shutdown();
    }

private:
    struct Participant {
        PendingClient connection;
        Player player;
        bool alive = true;
    };

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    std::size_t aliveCount() const {
        std::size_t count = 0;
        for (const auto& participant : m_players) {
            if (participant.alive) {
                ++count;
            }
        }
        return count;
    }

    std::size_t nextAliveFrom(std::size_t startIndex) const {
        if (m_players.empty()) {
            return npos;
        }

        for (std::size_t offset = 0; offset < m_players.size(); ++offset) {
            const std::size_t index = (startIndex + offset) % m_players.size();
            if (m_players[index].alive) {
                return index;
            }
        }

        return npos;
    }

    std::size_t nextOpponentAfter(std::size_t activeIndex) const {
        for (std::size_t offset = 1; offset < m_players.size(); ++offset) {
            const std::size_t index = (activeIndex + offset) % m_players.size();
            if (m_players[index].alive) {
                return index;
            }
        }

        return npos;
    }

    std::size_t resolveTarget(std::size_t activeIndex, const net::AttackCommand& command) const {
        if (m_players.size() < 2) {
            return npos;
        }

        if (m_players.size() == 2) {
            const std::size_t other = (activeIndex + 1) % 2;
            if (command.hasTarget && static_cast<std::size_t>(command.targetIndex) != other) {
                return npos;
            }
            return m_players[other].alive ? other : npos;
        }

        if (command.hasTarget) {
            if (command.targetIndex < 0) {
                return npos;
            }

            const std::size_t target = static_cast<std::size_t>(command.targetIndex);
            if (target >= m_players.size() || target == activeIndex || !m_players[target].alive) {
                return npos;
            }

            return target;
        }

        return nextOpponentAfter(activeIndex);
    }

    void eliminatePlayer(std::size_t playerIndex) {
        if (!m_players[playerIndex].alive) {
            return;
        }

        m_players[playerIndex].alive = false;
        net::sendLine(m_players[playerIndex].connection.socketFd, "LOSE");
        closeSocket(m_players[playerIndex].connection.socketFd);
    }

    void handleDisconnect(std::size_t playerIndex) {
        if (!m_players[playerIndex].alive) {
            return;
        }

        m_players[playerIndex].alive = false;
        closeSocket(m_players[playerIndex].connection.socketFd);
    }

    void declareWinner() {
        const std::size_t winner = nextAliveFrom(0);
        if (winner == npos) {
            return;
        }

        net::sendLine(m_players[winner].connection.socketFd, "WIN");
        closeSocket(m_players[winner].connection.socketFd);
    }

    void sendBoardsToAlive() {
        for (std::size_t index = 0; index < m_players.size(); ++index) {
            if (!m_players[index].alive) {
                continue;
            }

            const Board& board = m_players[index].player.board();
            net::sendLine(m_players[index].connection.socketFd, "BOARD_BEGIN OWN");
            for (const auto& line : board.renderLines(true)) {
                net::sendLine(m_players[index].connection.socketFd, line);
            }
            net::sendLine(m_players[index].connection.socketFd, "BOARD_END");
        }
    }

    void broadcast(const std::string& message) {
        for (const auto& participant : m_players) {
            if (participant.alive) {
                net::sendLine(participant.connection.socketFd, message);
            }
        }
    }

    std::string buildNameList() const {
        std::string names;
        for (const auto& participant : m_players) {
            names.push_back(' ');
            names += participant.player.name();
        }
        return names;
    }

    void notifyShutdown() {
        broadcast("SERVER_SHUTDOWN");
    }

    static void closeSocket(int& socketFd) {
        if (socketFd >= 0) {
            ::shutdown(socketFd, SHUT_RDWR);
            ::close(socketFd);
            socketFd = -1;
        }
    }

    std::vector<Participant> m_players;
    std::mt19937 m_rng{std::random_device{}()};
    std::atomic<bool> m_running{true};
};