#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <random>
#include "Player.h"
#include "Ship.h"

class Game {
public:
  Game(const std::string& p1, const std::string& p2): rng(std::random_device{}()) {
    players.emplace_back(p1);
    players.emplace_back(p2);
  }
  void setup() {
    std::vector<std::pair<std::string,int>> shipDefs = {
      {"Carrier",5},{"Battleship",4},{"Cruiser",3},{"Submarine",3},{"Destroyer",2}
    };
    for (auto &pl: players) {
      for (auto &d: shipDefs) {
        Ship s(d.first, d.second);
        bool ok = pl.board().placeShipRandom(s, rng);
        if (!ok) std::cerr<<"Failed to place "<<d.first<<" for "<<pl.name()<<"\n";
      }
    }
  }
  void play() {
    int turn=0;
    while (true) {
      Player &att = players[turn%2];
      Player &def = players[(turn+1)%2];
      std::cout<<"\n"<<att.name()<<"'s turn to attack"<<"\n";
      std::cout<<"Your board:\n"; att.board().displayOwn();
      std::cout<<"Opponent view:\n"; def.board().displayMask();
      std::string input;
      int r=-1,c=-1;
      while (true) {
        std::cout<<"Enter target (e.g. A5): ";
        if (!std::getline(std::cin,input)) return;
        if (parseCoord(input,r,c)) break;
        std::cout<<"Invalid input. Try again.\n";
      }
      std::string result;
      bool hit = def.board().receiveAttack(r,c,result);
      if (result=="Already") { std::cout<<"Already attacked that cell. Try again.\n"; continue; }
      std::cout<<"Result: "<<result<<"\n";
      if (def.board().allSunk()) { std::cout<<att.name()<<" wins!\n"; break; }
      ++turn;
    }
  }
private:
  bool parseCoord(const std::string &s, int &r, int &c) {
    if (s.empty()) return false;
    char ch=0; int col=-1;
    size_t i=0;
    while (i<s.size() && isspace((unsigned char)s[i])) ++i;
    if (i<s.size() && isalpha((unsigned char)s[i])) { ch = toupper((unsigned char)s[i]); ++i; }
    else return false;
    while (i<s.size() && isspace((unsigned char)s[i])) ++i;
    std::string num;
    while (i<s.size() && isdigit((unsigned char)s[i])) { num.push_back(s[i]); ++i; }
    if (num.empty()) return false;
    try { col = std::stoi(num); } catch(...) { return false; }
    r = ch - 'A'; c = col;
    if (r<0||r>=Board::N||c<0||c>=Board::N) return false;
    return true;
  }

  std::vector<Player> players;
  std::mt19937 rng;
};
