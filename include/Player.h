#pragma once

#include <string>
#include <utility>

#include "Board.h"

class Player {
public:
    explicit Player(std::string name) : m_name(std::move(name)) {}

    const std::string& name() const { return m_name; }
    Board& board() { return m_board; }
    const Board& board() const { return m_board; }

private:
    std::string m_name;
    Board m_board;
};