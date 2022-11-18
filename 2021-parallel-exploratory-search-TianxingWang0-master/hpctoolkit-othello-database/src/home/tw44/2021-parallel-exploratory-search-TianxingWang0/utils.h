#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_list.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_max.h>

#define BIT 0x1

#define X_BLACK 0
#define O_WHITE 1
#define OTHERCOLOR(c) (1 - (c))

/* 
	represent game board squares as a 64-bit unsigned integer.
	these macros index from a row,column position on the board
	to a position and bit in a game board bitvector
*/
#define BOARD_BIT_INDEX(row, col) ((8 - (row)) * 8 + (8 - (col)))
#define BOARD_BIT(row, col) (0x1LL << BOARD_BIT_INDEX(row, col))
#define MOVE_TO_BOARD_BIT(m) BOARD_BIT(m.row, m.col)

/* all of the bits in the row 8 */
#define ROW8                                                               \
  (BOARD_BIT(8, 1) | BOARD_BIT(8, 2) | BOARD_BIT(8, 3) | BOARD_BIT(8, 4) | \
   BOARD_BIT(8, 5) | BOARD_BIT(8, 6) | BOARD_BIT(8, 7) | BOARD_BIT(8, 8))

/* all of the bits in column 8 */
#define COL8                                                               \
  (BOARD_BIT(1, 8) | BOARD_BIT(2, 8) | BOARD_BIT(3, 8) | BOARD_BIT(4, 8) | \
   BOARD_BIT(5, 8) | BOARD_BIT(6, 8) | BOARD_BIT(7, 8) | BOARD_BIT(8, 8))

/* all of the bits in column 1 */
#define COL1 (COL8 << 7)

#define IS_MOVE_OFF_BOARD(m) (m.row < 1 || m.row > 8 || m.col < 1 || m.col > 8)
#define IS_DIAGONAL_MOVE(m) (m.row != 0 && m.col != 0)
#define MOVE_OFFSET_TO_BIT_OFFSET(m) (m.row * 8 + m.col)

#define INPUT_FILE "input"

typedef unsigned long long ull;

/* 
	game board represented as a pair of bit vectors: 
	- one for x_black disks on the board
	- one for o_white disks on the board
*/
typedef struct
{
  ull disks[2];
} Board;

/*
Overload < operator to choose one from equally optimal candidates
*/
typedef struct Move {
  int row = 0;
  int col = 0;
  Move() {}
  Move(int r, int c) : row(r), col(c) {}
  Move(const Move& other) : row(other.row), col(other.col) {}
  
  bool operator < (const Move& other) const {
    if (row == other.row) {
      return col < other.col;
    }
    return row < other.row;
  }

} Move;

extern Board start;
extern Move offsets[8];
extern int noffsets;
extern char diskcolor[4];

typedef struct {
  bool ishuman = true;
  int depth;
} Player;

void PrintDisk(int x_black, int o_white);

void PrintBoardRow(int x_black, int o_white, int disks);

void PrintBoardRows(ull x_black, ull o_white, int rowsleft);

void PrintBoard(Board b);

/* 
	place a disk of color at the position specified by m.row and m,col,
	flipping the opponents disk there (if any) 
*/
void PlaceOrFlip(Move m, Board *b, int color);

/* 
	try to flip disks along a direction specified by a move offset.
	the return code is 0 if no flips were done.
	the return value is 1 + the number of flips otherwise.
*/
int TryFlips(const Move& m, const Move& offset, Board *b, int color, int verbose, int domove);

int FlipDisks(const Move& m, Board *b, int color, int verbose, int domove);

bool ReadPlayer(Player& player);

bool ReadPlayers(Player& x_black, Player& y_white);

void ReadMove(int color, Board *b);

/*
	return the set of board positions adjacent to an opponent's
	disk that are empty. these represent a candidate set of 
	positions for a move by color.
*/
Board NeighborMoves(Board b, int color);

/*
	return the set of board positions that represent legal
	moves for color. this is the set of empty board positions  
	that are adjacent to an opponent's disk where placing a
	disk of color will cause one or more of the opponent's
	disks to be flipped.
*/
int EnumerateLegalMoves(Board b, int color, Board *legal_moves);

int HumanTurn(Board *b, int color);

int CountBitsOnBoard(Board *b, int color);

void EndGame(Board b);