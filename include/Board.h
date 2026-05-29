#pragma once

#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Ship.h"

class Board {
public:
    static constexpr int Size = 10;

    enum class Cell {
        Empty,
        ShipCell,
        Hit,
        Miss,
    };

    enum class AttackOutcome {
        Invalid,
        Already,
        Miss,
        Hit,
        Sunk,
    };

    struct AttackReport {
        AttackOutcome outcome = AttackOutcome::Invalid;
        std::string message;
    };

    Board() : m_grid(Size, std::vector<Cell>(Size, Cell::Empty)) {}

    bool placeRandomFleet(std::mt19937& rng) {
        const std::vector<std::pair<std::string, int>> shipDefs = {
            {"Carrier", 5},
            {"Battleship", 4},
            {"Cruiser", 3},
            {"Submarine", 3},
            {"Destroyer", 2},
        };

        for (const auto& shipDef : shipDefs) {
            Ship ship(shipDef.first, shipDef.second);
            if (!placeShipRandom(ship, rng)) {
                return false;
            }
        }

        return true;
    }

    AttackReport attack(int row, int col) {
        if (!inBounds(row, col)) {
            return {AttackOutcome::Invalid, "Invalid"};
        }

        if (m_grid[row][col] == Cell::Hit || m_grid[row][col] == Cell::Miss) {
            return {AttackOutcome::Already, "Already"};
        }

        if (m_grid[row][col] == Cell::ShipCell) {
            m_grid[row][col] = Cell::Hit;

            for (auto& ship : m_ships) {
                if (!ship.occupies(row, col)) {
                    continue;
                }

                ship.hitAt(row, col);
                if (ship.isSunk()) {
                    return {AttackOutcome::Sunk, std::string("Sunk ") + ship.name()};
                }

                return {AttackOutcome::Hit, "Hit"};
            }

            return {AttackOutcome::Hit, "Hit"};
        }

        m_grid[row][col] = Cell::Miss;
        return {AttackOutcome::Miss, "Miss"};
    }

    bool allSunk() const {
        for (const auto& ship : m_ships) {
            if (!ship.isSunk()) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::string> renderLines(bool revealShips) const {
        std::vector<std::string> lines;

        std::ostringstream header;
        header << "   ";
        for (int column = 0; column < Size; ++column) {
            header << column << ' ';
        }
        lines.push_back(header.str());

        for (int row = 0; row < Size; ++row) {
            std::ostringstream line;
            line << static_cast<char>('A' + row) << "  ";
            for (int col = 0; col < Size; ++col) {
                char cell = '.';
                if (m_grid[row][col] == Cell::Hit) {
                    cell = 'X';
                } else if (m_grid[row][col] == Cell::Miss) {
                    cell = 'o';
                } else if (revealShips && m_grid[row][col] == Cell::ShipCell) {
                    cell = 'S';
                }

                line << cell << ' ';
            }
            lines.push_back(line.str());
        }

        return lines;
    }

private:
    bool placeShipRandom(Ship& ship, std::mt19937& rng) {
        std::uniform_int_distribution<int> position(0, Size - 1);
        std::uniform_int_distribution<int> direction(0, 1);

        for (int attempt = 0; attempt < 1000; ++attempt) {
            const int row = position(rng);
            const int col = position(rng);
            const bool horizontal = direction(rng) == 1;

            if (!canPlace(row, col, ship.size(), horizontal)) {
                continue;
            }

            std::vector<std::pair<int, int>> cells;
            for (int index = 0; index < ship.size(); ++index) {
                const int placedRow = row + (horizontal ? 0 : index);
                const int placedCol = col + (horizontal ? index : 0);
                m_grid[placedRow][placedCol] = Cell::ShipCell;
                cells.emplace_back(placedRow, placedCol);
            }

            ship.setCells(cells);
            m_ships.push_back(ship);
            return true;
        }

        return false;
    }

    bool canPlace(int row, int col, int size, bool horizontal) const {
        if (!inBounds(row, col)) {
            return false;
        }

        if (horizontal) {
            if (col + size > Size) {
                return false;
            }

            for (int index = 0; index < size; ++index) {
                if (m_grid[row][col + index] != Cell::Empty) {
                    return false;
                }
            }
        } else {
            if (row + size > Size) {
                return false;
            }

            for (int index = 0; index < size; ++index) {
                if (m_grid[row + index][col] != Cell::Empty) {
                    return false;
                }
            }
        }

        return true;
    }

    bool inBounds(int row, int col) const {
        return row >= 0 && row < Size && col >= 0 && col < Size;
    }

    std::vector<std::vector<Cell>> m_grid;
    std::vector<Ship> m_ships;
};