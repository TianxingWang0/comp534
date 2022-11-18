#include "utils.h"
#include "timer.h"
#include "computer.h"

int main(int argc, const char *argv[]) {
  Player x_black, o_white;
  while (!ReadPlayers(x_black, o_white)) {
    printf("Invalid player setting, try again.\n");
  }
  Board gameboard = start;
  int move_possible;
  PrintBoard(gameboard);
  // measure the real time for the calculation 
  timer_start();
  if (x_black.ishuman) {
    if (o_white.ishuman) {
      do {
        move_possible =
            HumanTurn(&gameboard, X_BLACK) |
            HumanTurn(&gameboard, O_WHITE);
      } while (move_possible);
    } else {
      do {
        move_possible =
            HumanTurn(&gameboard, X_BLACK) |
            ComputerTurn(&gameboard, O_WHITE, o_white.depth);
      } while (move_possible);
    }
  } else {
    if (o_white.ishuman) {
      do {
        move_possible =
            ComputerTurn(&gameboard, X_BLACK, x_black.depth) |
            HumanTurn(&gameboard, O_WHITE);
      } while (move_possible);
    } else {
      do {
        move_possible =
            ComputerTurn(&gameboard, X_BLACK, x_black.depth) |
            ComputerTurn(&gameboard, O_WHITE, o_white.depth);
      } while (move_possible);
      printf("Depth of X : %d, O : %d\n", x_black.depth, o_white.depth);
    }
  }
  double seconds = timer_elapsed();
  EndGame(gameboard);
  printf("%d workers in %.3f seconds\n", __cilkrts_get_nworkers(), seconds);
  return 0;
}
