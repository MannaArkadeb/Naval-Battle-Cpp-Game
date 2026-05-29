#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "MatchSession.h"

class BattleshipServer {
public:
    BattleshipServer(int port, std::size_t playersPerMatch)
        : m_port(port), m_playersPerMatch(std::max<std::size_t>(2, playersPerMatch)) {}

    ~BattleshipServer() {
        stop();
    }

    bool start() {
        m_listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenSocket < 0) {
            std::cerr << "Failed to create listening socket.\n";
            return false;
        }

        int reuse = 1;
        ::setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(static_cast<uint16_t>(m_port));

        if (::bind(m_listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            std::cerr << "Failed to bind port " << m_port << ".\n";
            ::close(m_listenSocket);
            m_listenSocket = -1;
            return false;
        }

        if (::listen(m_listenSocket, 8) < 0) {
            std::cerr << "Failed to listen on port " << m_port << ".\n";
            ::close(m_listenSocket);
            m_listenSocket = -1;
            return false;
        }

        m_running.store(true);
        m_acceptThread = std::thread(&BattleshipServer::acceptLoop, this);
        m_gameThread = std::thread(&BattleshipServer::matchLoop, this);

        std::cout << "Server listening on port " << m_port << " with match size " << m_playersPerMatch << "\n";
        return true;
    }

    void stop() {
        const bool wasRunning = m_running.exchange(false);
        if (!wasRunning && m_listenSocket < 0 && !m_acceptThread.joinable() && !m_gameThread.joinable()) {
            return;
        }

        if (m_listenSocket >= 0) {
            ::shutdown(m_listenSocket, SHUT_RDWR);
            ::close(m_listenSocket);
            m_listenSocket = -1;
        }

        m_queueCv.notify_all();

        if (m_acceptThread.joinable()) {
            m_acceptThread.join();
        }

        if (m_gameThread.joinable()) {
            m_gameThread.join();
        }

        {
            std::lock_guard<std::mutex> lock(m_matchMutex);
            for (auto& match : m_activeMatches) {
                match.session->shutdown();
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_matchMutex);
            for (auto& match : m_activeMatches) {
                if (match.worker.joinable()) {
                    match.worker.join();
                }
            }
            m_activeMatches.clear();
        }

        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_waitingPlayers.empty()) {
            ::close(m_waitingPlayers.front().socketFd);
            m_waitingPlayers.pop();
        }
    }

private:
    struct ActiveMatch {
        std::shared_ptr<MatchSession> session;
        std::thread worker;
    };

    void acceptLoop() {
        while (m_running.load()) {
            sockaddr_in clientAddress{};
            socklen_t clientLength = sizeof(clientAddress);

            const int clientSocket = ::accept(m_listenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
            if (clientSocket < 0) {
                if (!m_running.load()) {
                    break;
                }
                continue;
            }

            std::string line;
            std::string name;
            if (!net::recvLine(clientSocket, line) || !net::parseJoin(line, name)) {
                net::sendLine(clientSocket, "ERROR EXPECTED JOIN NAME");
                ::close(clientSocket);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_waitingPlayers.push(PendingClient{clientSocket, name});
            }

            net::sendLine(clientSocket, "QUEUED " + name);
            m_queueCv.notify_one();
        }
    }

    void matchLoop() {
        while (m_running.load()) {
            std::vector<PendingClient> group;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCv.wait(lock, [this] {
                    return !m_running.load() || m_waitingPlayers.size() >= m_playersPerMatch;
                });

                if (!m_running.load()) {
                    return;
                }

                for (std::size_t index = 0; index < m_playersPerMatch; ++index) {
                    group.push_back(m_waitingPlayers.front());
                    m_waitingPlayers.pop();
                }
            }

            auto session = std::make_shared<MatchSession>(std::move(group));
            ActiveMatch match;
            match.session = session;
            match.worker = std::thread([session] {
                session->run();
            });

            std::lock_guard<std::mutex> lock(m_matchMutex);
            m_activeMatches.push_back(std::move(match));
        }
    }

    int m_port = 0;
    std::size_t m_playersPerMatch = 2;
    int m_listenSocket = -1;
    std::atomic<bool> m_running{false};
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::queue<PendingClient> m_waitingPlayers;
    std::mutex m_matchMutex;
    std::vector<ActiveMatch> m_activeMatches;
    std::thread m_acceptThread;
    std::thread m_gameThread;
};