#pragma once

#include <list>
#include "utils.h"

/*
Use cilk reducer to get next move
*/

/*
Computer make one move
*/
int ComputerTurn(Board *b, int color, int depth);

/*
Computer's move with this color 
calculate the Minimax best move and place move on board
*/
int GenerateMove(int color, Board *b, int depth);


/*
For same evaluation value candidates, choose the one with biggest
row index and biggest col index
*/
typedef struct Value {
    Move m;
    int value;

    Value(){}
    Value(const Move& om, int v) : m(om), value(v) {}
    Value(const Value& other) : m(other.m), value(other.value) {}

    bool operator < (const Value& other) const {
        if (value == other.value) {
            return m < other.m;
        }
        return value < other.value;
    }
} Value;

// Three functions for Minimax Algorithm

/*
At X's move, try to maximize the score
parameter m is the previous move of O
*/
Value MaxValue(Board* bptr, int depth, bool pre_moved);

/*
At O's move, try to minimize the score
parameter m is the previous move of X
*/
Value MinValue(Board* bptr, int depth, bool pre_moved);

/*
Evaluation function for Minimax algorithm, return the
eveluated score of current state, the score is 
the number of X on board subtract with the number of O on board

*/
int Evaluate(Board* b);