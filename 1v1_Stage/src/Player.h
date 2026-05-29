#pragma once
#include <string>
#include "Board.h"

class Player {
public:
  Player() = default;
  Player(const std::string& name): m_name(name) {}
  Board& board() { return m_board; }
  const std::string& name() const { return m_name; }
private:
  std::string m_name;
  Board m_board;
};
