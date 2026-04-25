#include "CTable.h"
#include <stdio.h>
#include <iostream>

CTable::CTable(CPlayer* p1, CPlayer* p2, CDominoes* d)
    : player1(p1), player2(p2), dominoes(d), head(-1), tail(-1), consecutivePasses(0) {
    pthread_mutex_init(&tableMutex, NULL);
}

CTable::~CTable() {
    pthread_mutex_destroy(&tableMutex);
}

void CTable::API() {
    create_dominoes();
    draw_dominoes();
    int current = who_first();
    first_piece(current);
    run_game(current);
    final_report();
}

void CTable::create_dominoes() {
    dominoes->create_dominoes();
    shuffleVec(dominoes->pieces());
}

void CTable::draw_dominoes() {
    std::vector<Domino>& deck = dominoes->pieces();

    for (int i = 0; i < 10; ++i) {
        player1->addToHand(deck.back()); deck.pop_back();
        player2->addToHand(deck.back()); deck.pop_back();
    }

    std::vector<Domino> by;
    while (!deck.empty()) {
        by.push_back(deck.back());
        deck.pop_back();
    }
    dominoes->setBoneyard(by);

    printf("=== Initial Deal ===\n");
    player1->printHand();
    player2->printHand();
    printf("Boneyard size: %d\n\n", dominoes->boneyardSize());
}

int CTable::who_first() {
    int first = randInt(0, 1);
    printf("=== Who goes first? ===\n");
    printf("Random first player: %s\n\n", first == 0 ? player1->getName() : player2->getName());
    return first;
}

void CTable::first_piece(int& currentPlayer) {
    CPlayer* p = (currentPlayer == 0 ? player1 : player2);
    int bestIdx = 0;

    for (int i = 1; i < p->handSize(); ++i) {
        if (p->getHand()[i].sum() > p->getHand()[bestIdx].sum()) bestIdx = i;
    }

    Domino first = p->removeAt(bestIdx);
    train.clear();
    train.push_back(first);
    head = first.left;
    tail = first.right;

    printf("=== First Piece ===\n");
    printf("%s starts with ", p->getName());
    first.print();
    printf("\n");
    printTrain();

    currentPlayer = 1 - currentPlayer;
}

void* CTable::playerThread(void* arg) {
    TurnData* data = static_cast<TurnData*>(arg);

    std::cerr << std::endl << "Player " << data->player->getName() << " acquiring lock on game data" << std::endl << std::endl;
    pthread_mutex_lock(&data->table->tableMutex);
    data->played = data->table->take_turn(*data->player);
    std::cerr << std::endl << "Player " << data->player->getName() << " releasing lock on game data" << std::endl;
    pthread_mutex_unlock(&data->table->tableMutex);

    return NULL;
}

void CTable::run_game(int currentPlayer) {
    printf("\n=== Run Game: POSIX threads + mutex protected turns ===\n");

    while (player1->handSize() > 0 && player2->handSize() > 0 && consecutivePasses < 2) {
        CPlayer* p = (currentPlayer == 0 ? player1 : player2);

        printf("\n-- Turn: %s --\n", p->getName());
        p->printHand();
        printf("Head=%d Tail=%d | Boneyard=%d\n", head, tail, dominoes->boneyardSize());

        TurnData data;
        data.table = this;
        data.player = p;
        data.played = false;

        pthread_t tid;
        pthread_create(&tid, NULL, CTable::playerThread, &data);
        pthread_join(tid, NULL);

        consecutivePasses = data.played ? 0 : consecutivePasses + 1;
        currentPlayer = 1 - currentPlayer;
    }
}

bool CTable::take_turn(CPlayer& p) {
    CPlayer::PlayChoice choice = p.findMatch(head, tail);

    while (!choice.found && dominoes->canDraw()) {
        Domino drawn = dominoes->draw();
        p.addToHand(drawn);
        printf("%s draws ", p.getName());
        drawn.print();
        printf("\n");
        choice = p.findMatch(head, tail);
    }

    if (!choice.found) {
        printf("%s cannot play and passes.\n", p.getName());
        return false;
    }

    Domino toPlay = p.removeAt(choice.idx);
    if (choice.flip) toPlay.flip();

    if (choice.placeEnd == 0) {
        train.insert(train.begin(), toPlay);
        head = toPlay.left;
        printf("%s plays at HEAD: ", p.getName());
    } else {
        train.push_back(toPlay);
        tail = toPlay.right;
        printf("%s plays at TAIL: ", p.getName());
    }

    toPlay.print();
    if (choice.flip) printf(" (flipped)");
    printf("\nPlayed tile:\n");
    toPlay.printAscii();
    printTrain();
    return true;
}

void CTable::printTrain() const {
    printf("Train: ");
    for (size_t i = 0; i < train.size(); ++i) {
        train[i].print();
        printf(" ");
    }
    printf("\n");
}

void CTable::final_report() {
    printf("\n=== Final Result ===\n");

    const char* winner = "TIE";
    if (player1->handSize() == 0) winner = player1->getName();
    else if (player2->handSize() == 0) winner = player2->getName();
    else {
        int sum1 = 0, sum2 = 0;
        for (size_t i = 0; i < player1->getHand().size(); ++i) sum1 += player1->getHand()[i].sum();
        for (size_t i = 0; i < player2->getHand().size(); ++i) sum2 += player2->getHand()[i].sum();

        if (sum1 < sum2) winner = player1->getName();
        else if (sum2 < sum1) winner = player2->getName();
    }

    printf("Winner: %s\n\n", winner);
    printf("Final Train Placed:\n");
    printTrain();
    player1->printHand();
    player2->printHand();
    printf("Boneyard remaining: %d\n", dominoes->boneyardSize());
}
