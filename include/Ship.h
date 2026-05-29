#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class Ship {
public:
    Ship() = default;
    Ship(const std::string& name, int size) : m_name(name), m_size(size), m_hits(size, false) {}

    int size() const { return m_size; }
    const std::string& name() const { return m_name; }

    void setCells(const std::vector<std::pair<int, int>>& cells) {
        m_cells = cells;
        m_hits.assign(m_size, false);
    }

    bool occupies(int row, int col) const {
        for (const auto& cell : m_cells) {
            if (cell.first == row && cell.second == col) {
                return true;
            }
        }
        return false;
    }

    bool hitAt(int row, int col) {
        for (std::size_t index = 0; index < m_cells.size(); ++index) {
            if (m_cells[index].first == row && m_cells[index].second == col) {
                m_hits[index] = true;
                return true;
            }
        }
        return false;
    }

    bool isSunk() const {
        for (bool hit : m_hits) {
            if (!hit) {
                return false;
            }
        }
        return true;
    }

private:
    std::string m_name;
    int m_size = 0;
    std::vector<std::pair<int, int>> m_cells;
    std::vector<bool> m_hits;
};