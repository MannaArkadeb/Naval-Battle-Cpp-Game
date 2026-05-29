#pragma once
#include <vector>
#include <utility>
#include <string>

class Ship {
public:
    Ship() = default;
    Ship(const std::string& name, int size): m_name(name), m_size(size), m_hits(size,false) {}
    int size() const { return m_size; }
    const std::string& name() const { return m_name; }
    void setCells(const std::vector<std::pair<int,int>>& cells) { m_cells = cells; m_hits.assign(m_size,false); }
    const std::vector<std::pair<int,int>>& cells() const { return m_cells; }
    bool occupies(int r,int c) const {
        for (auto &p: m_cells) if (p.first==r && p.second==c) return true;
        return false;
    }
    bool hitAt(int r,int c) {
        for (size_t i=0;i<m_cells.size();++i) if (m_cells[i].first==r && m_cells[i].second==c) { m_hits[i]=true; return true; }
        return false;
    }
    bool isSunk() const {
        if (m_hits.empty()) return true;
        for (bool h: m_hits) if (!h) return false;
        return true;
    }
private:
    std::string m_name;
    int m_size=0;
    std::vector<std::pair<int,int>> m_cells;
    std::vector<bool> m_hits;
    friend class Board;
};
