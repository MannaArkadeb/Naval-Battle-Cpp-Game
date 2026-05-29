#include <iostream>
#include "Game.h"

int main() {
    std::cout << "1 vs 1 Battleship - Offline Local Game\n";
    std::cout << "Objective: sink all opponent ships.\n";
    std::cout << "Input: coordinates like A5 (letter row + number column).\n";
    std::cout << "Take turns; enter one target per turn.\n\n";
    std::cout << "Note: On your board 'S' shows your ships.\n";
    std::cout << "Opponent view hides 'S' and shows 'X' for hits and 'o' for misses.\n\n";
    std::string p1, p2;
    std::cout << "Enter name for Player 1: "; std::getline(std::cin, p1);
    if (p1.empty()) p1 = "Player 1";
    std::cout << "Enter name for Player 2: "; std::getline(std::cin, p2);
    if (p2.empty()) p2 = "Player 2";

    Game game(p1, p2);
    game.setup();
    game.play();

    return 0;
}
