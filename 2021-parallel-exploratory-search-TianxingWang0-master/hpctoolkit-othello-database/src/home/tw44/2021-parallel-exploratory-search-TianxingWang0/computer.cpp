#include "computer.h"

#include <algorithm>

int ComputerTurn(Board *b, int color, int depth) {
  return GenerateMove(color, b, depth);
}

int GenerateMove(int color, Board *b, int depth) {
  Board neighbors = NeighborMoves(*b, color);
  Move best_move;
  if (color == X_BLACK) {  // for X, maximize the minimum
    best_move = MaxValue(b, depth).m;
  } else {                 // for O, minimize the maximum
    best_move = MinValue(b, depth).m;
  }
  if (best_move.row == 0) {  // cannot move in this turn
    return 0;
  }
  int nflips = FlipDisks(best_move, b, color, 1, 1);
  PlaceOrFlip(best_move, b, color);
  printf("Computer %c flipped %d disks\n", (color == X_BLACK ? 'X' : 'O'), nflips);
  PrintBoard(*b);
  return 1;
}

Value MaxValue(Board* bptr, int depth) {
  if (depth) {
    Board neighbors = NeighborMoves(*bptr, X_BLACK);
    --depth;
    cilk::reducer_max<Value> max_value({{}, -65});
    cilk_for (int row = 8; row >= 1; row--) {
      ull my_neighbor_moves = neighbors.disks[X_BLACK] >> (8 * (8 - row));
      ull thisrow = my_neighbor_moves & ROW8;
      for (int col = 8; thisrow && (col >= 1); col--) {
        if (thisrow & COL8) {
          Move m = {row, col};
          if (FlipDisks(m, bptr, X_BLACK, 0, 0) > 0) {
            Board b(*bptr);
            FlipDisks(m, &b, X_BLACK, 0, 1);
            PlaceOrFlip(m, &b, X_BLACK);
            // printf("X generate move\n");
            // PrintBoard(&b);
            max_value.calc_max({m, MinValue(&b, depth).value});
          }
        }
        thisrow >>= 1;
      }
    }
    if (max_value.get_value().value >= -64) {
      return max_value.get_value();
    }
  }
  return {{0, 0}, Evaluate(bptr)};
}

Value MinValue(Board* bptr, int depth) {
  if (depth) {
    Board neighbors = NeighborMoves(*bptr, O_WHITE);
    cilk::reducer_min<Value> min_value({{}, 65});
    depth--;
    cilk_for (int row = 8; row >= 1; row--) {
      ull my_neighbor_moves = neighbors.disks[O_WHITE] >> (8 * (8 - row));
      ull thisrow = my_neighbor_moves & ROW8;
      for (int col = 8; thisrow && (col >= 1); col--) {
        if (thisrow & COL8) {
          Move m = {row, col};
          if (FlipDisks(m, bptr, O_WHITE, 0, 0) > 0) {
            Board b(*bptr);
            FlipDisks(m, &b, O_WHITE, 0, 1);
            PlaceOrFlip(m, &b, O_WHITE);
            // printf("X generate move\n");
            // PrintBoard(&b);
            min_value.calc_min({m, MaxValue(&b, depth).value});
          }
        }
        thisrow >>= 1;
      }
    }
    if (min_value.get_value().value <= 64) {
      return min_value.get_value();
    }
  }
  return {{0, 0}, Evaluate(bptr)};
}

int Evaluate(Board* b) {
  return CountBitsOnBoard(b, X_BLACK) - CountBitsOnBoard(b, O_WHITE);
}