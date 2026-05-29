#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <random>
#include "Ship.h"

class Board {
public:
  static const int N=10;
  enum Cell { Empty=0, ShipCell=1, Hit=2, Miss=3 };
  Board(): grid(N, std::vector<int>(N, Empty)) {}
  bool canPlace(int r,int c,int size,bool horiz) const {
    if (r<0||c<0) return false;
    if (horiz) {
      if (c+size> N) return false;
      for (int i=0;i<size;++i) if (grid[r][c+i]!=Empty) return false;
    } else {
      if (r+size> N) return false;
      for (int i=0;i<size;++i) if (grid[r+i][c]!=Empty) return false;
    }
    return true;
  }
  bool placeShipRandom(Ship &ship, std::mt19937 &rng) {
    std::uniform_int_distribution<int> dist(0,N-1);
    std::uniform_int_distribution<int> dir(0,1);
    for (int attempt=0;attempt<1000;++attempt) {
      int r=dist(rng), c=dist(rng); bool horiz = dir(rng)==1;
      if (!canPlace(r,c,ship.size(),horiz)) continue;
      std::vector<std::pair<int,int>> cells;
      for (int i=0;i<ship.size();++i) {
        int rr=r + (horiz?0:i);
        int cc=c + (horiz?i:0);
        grid[rr][cc]=ShipCell;
        cells.push_back({rr,cc});
      }
      ship.setCells(cells);
      ships.push_back(ship);
      return true;
    }
    return false;
  }
  bool receiveAttack(int r,int c, std::string &out) {
    if (r<0||r>=N||c<0||c>=N) { out="Invalid"; return false; }
    if (grid[r][c]==Hit||grid[r][c]==Miss) { out="Already"; return false; }
    if (grid[r][c]==ShipCell) {
      grid[r][c]=Hit;
      for (auto &s: ships) {
        if (s.occupies(r,c)) {
          s.hitAt(r,c);
          if (s.isSunk()) { out = std::string("Sunk ")+s.name(); } else { out = "Hit"; }
          break;
        }
      }
      return true;
    } else {
      grid[r][c]=Miss;
      out="Miss";
      return false;
    }
  }
  void displayOwn() const {
    std::cout<<"   "; for (int c=0;c<N;++c) std::cout<<c<<" "; std::cout<<"\n";
    for (int r=0;r<N;++r) {
      std::cout<<char('A'+r)<<"  "; 
      for (int c=0;c<N;++c) {
        char ch='.'; 
        if (grid[r][c]==ShipCell) ch='S';
        else if (grid[r][c]==Hit) ch='X';
        else if (grid[r][c]==Miss) ch='o';
        std::cout<<ch<<" "; 
      }
      std::cout<<"\n";
    }
  }
  void displayMask() const {
    std::cout<<"   "; for (int c=0;c<N;++c) std::cout<<c<<" "; std::cout<<"\n";
    for (int r=0;r<N;++r) {
      std::cout<<char('A'+r)<<"  "; 
      for (int c=0;c<N;++c) {
        char ch='.'; 
        if (grid[r][c]==Hit) ch='X';
        else if (grid[r][c]==Miss) ch='o';
        std::cout<<ch<<" "; 
      }
      std::cout<<"\n";
    }
  }
  bool allSunk() const {
    for (auto &s: ships) if (!s.isSunk()) return false;
    return true;
  }
private:
  std::vector<std::vector<int>> grid;
  std::vector<Ship> ships;
};
