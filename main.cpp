/*
Author: Abdulhamid Oguntade, Kris Heck, Salahudin Saleh
Course: CSCI 300
Combo 2 - Option 1: Dominoes Game using POSIX multi-threading

Build:
g++ -std=c++17 -Wall -Wextra -pthread main.cpp CDominoes.cpp CPlayer.cpp CRandom.cpp CTable.cpp -o index

Run:
./index
*/

#include "CDominoes.h"
#include "CPlayer.h"
#include "CTable.h"

int main() {
    CPlayer p1("P1");
    CPlayer p2("P2");
    CDominoes dominoes;

    CTable table(&p1, &p2, &dominoes);
    table.API();

    return 0;
}
