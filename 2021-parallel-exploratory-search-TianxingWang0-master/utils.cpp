#include "utils.h"
#include <fstream>
#include <iostream>

Board start = {
  BOARD_BIT(4, 5) | BOARD_BIT(5, 4) /* X_BLACK */,
  BOARD_BIT(4, 4) | BOARD_BIT(5, 5) /* O_WHITE */
};

Move offsets[] = {
  {0, 1} /* right */, {0, -1} /* left */, {-1, 0} /* up */, {1, 0} /* down */, {-1, -1} /* up-left */, {-1, 1} /* up-right */, {1, 1} /* down-right */, {1, -1} /* down-left */
};

int noffsets = sizeof(offsets) / sizeof(Move);
char diskcolor[] = {'.', 'X', 'O', 'I'};

void PrintDisk(int x_black, int o_white) {
  printf(" %c", diskcolor[x_black + (o_white << 1)]);
}

void PrintBoardRow(int x_black, int o_white, int disks) {
  if (disks > 1) {
    PrintBoardRow(x_black >> 1, o_white >> 1, disks - 1);
  }
  PrintDisk(x_black & BIT, o_white & BIT);
}

void PrintBoardRows(ull x_black, ull o_white, int rowsleft) {
  if (rowsleft > 1) {
    PrintBoardRows(x_black >> 8, o_white >> 8, rowsleft - 1);
  }
  printf("%d", rowsleft);
  PrintBoardRow((int)(x_black & ROW8), (int)(o_white & ROW8), 8);
  printf("\n");
}

void PrintBoard(Board b) {
  printf("  1 2 3 4 5 6 7 8\n");
  PrintBoardRows(b.disks[X_BLACK], b.disks[O_WHITE], 8);
}

void PlaceOrFlip(Move m, Board *b, int color) {
  ull bit = MOVE_TO_BOARD_BIT(m);
  b->disks[color] |= bit;
  b->disks[OTHERCOLOR(color)] &= ~bit;
}

int TryFlips(const Move& m, const Move& offset, Board *b, int color, int verbose, int domove) {
  Move next;
  next.row = m.row + offset.row;
  next.col = m.col + offset.col;

  if (!IS_MOVE_OFF_BOARD(next)) {
    ull nextbit = MOVE_TO_BOARD_BIT(next);
    if (nextbit & b->disks[OTHERCOLOR(color)]) {
      int nflips = TryFlips(next, offset, b, color, verbose, domove);
      if (nflips) {
        if (verbose) {
          printf("flipping disk at %d,%d\n", next.row, next.col);
        }
        if (domove) {
          PlaceOrFlip(next, b, color);
        }
        return nflips + 1;
      }
    } else if (nextbit & b->disks[color]) {
      return 1;
    }
  }
  return 0;
}

int FlipDisks(const Move& m, Board *b, int color, int verbose, int domove) {
  int i;
  int nflips = 0;

  /* try flipping disks along each of the 8 directions */
  for (i = 0; i < noffsets; i++) {
    int flipresult = TryFlips(m, offsets[i], b, color, verbose, domove);
    nflips += (flipresult > 0) ? flipresult - 1 : 0;
  }
  return nflips;
}

bool ReadPlayer(Player& player, int color) {
    char c;
    printf("Enter type(h or c) for player %c :\n", (color == X_BLACK ? 'X' : 'O'));
    scanf("%s", &c);
    if (c == 'c') {
      printf("Enter the depth of the computer player(1~60) :\n");
      int depth;
      scanf("%d", &depth);
      if (depth >= 1 && depth <= 60) {
          player.ishuman = false;
          player.depth = depth;
      } else {
          return false;
      }
    } else if (c == 'h') {
        player.ishuman = true;
    } else {
        return false;
    }
    return true;
}

bool ReadPlayers(Player& x_black, Player& o_white) {
    return ReadPlayer(x_black, X_BLACK) && ReadPlayer(o_white, O_WHITE);
}

void ReadMove(int color, Board *b) {
  Move m;
  ull movebit;
  for (;;) {
    printf("Enter %c's move as 'row,col': ", diskcolor[color + 1]);
    scanf("%d,%d", &m.row, &m.col);

    /* if move is not on the board, move again */
    if (IS_MOVE_OFF_BOARD(m)) {
      printf("Illegal move: row and column must both be between 1 and 8\n");
      PrintBoard(*b);
      continue;
    }
    movebit = MOVE_TO_BOARD_BIT(m);

    /* if board position occupied, move again */
    if (movebit & (b->disks[X_BLACK] | b->disks[O_WHITE])) {
      printf("Illegal move: board position already occupied.\n");
      PrintBoard(*b);
      continue;
    }

    /* if no disks have been flipped */
    {
      int nflips = FlipDisks(m, b, color, 1, 1);
      if (nflips == 0) {
        printf("Illegal move: no disks flipped\n");
        PrintBoard(*b);
        continue;
      }
      PlaceOrFlip(m, b, color);
      printf("You flipped %d disks\n", nflips);
      PrintBoard(*b);
    }
    break;
  }
}

Board NeighborMoves(Board b, int color) {
  int i;
  Board neighbors = {0, 0};
  for (i = 0; i < noffsets; i++) {
    ull colmask = (offsets[i].col != 0) ? ((offsets[i].col > 0) ? COL1 : COL8) : 0;
    int offset = MOVE_OFFSET_TO_BIT_OFFSET(offsets[i]);

    if (offset > 0) {
      neighbors.disks[color] |=
          (b.disks[OTHERCOLOR(color)] >> offset) & ~colmask;
    } else {
      neighbors.disks[color] |=
          (b.disks[OTHERCOLOR(color)] << -offset) & ~colmask;
    }
  }
  neighbors.disks[color] &= ~(b.disks[X_BLACK] | b.disks[O_WHITE]);
  return neighbors;
}

int EnumerateLegalMoves(Board b, int color, Board *legal_moves) {
  static Board no_legal_moves = {0, 0};
  Board neighbors = NeighborMoves(b, color);
  ull my_neighbor_moves = neighbors.disks[color];
  int row;
  int col;

  int num_moves = 0;
  *legal_moves = no_legal_moves;

  for (row = 8; row >= 1; row--) {
    ull thisrow = my_neighbor_moves & ROW8;
    for (col = 8; thisrow && (col >= 1); col--) {
      if (thisrow & COL8) {
        Move m = {row, col};
        if (FlipDisks(m, &b, color, 0, 0) > 0) {
          legal_moves->disks[color] |= BOARD_BIT(row, col);
          num_moves++;
        }
      }
      thisrow >>= 1;
    }
    my_neighbor_moves >>= 8;
  }
  return num_moves;
}

int HumanTurn(Board *b, int color) {
  Board legal_moves;
  int num_moves = EnumerateLegalMoves(*b, color, &legal_moves);
  if (num_moves > 0) {
    ReadMove(color, b);
    return 1;
  } else {
    return 0;
  }
}

int CountBitsOnBoard(Board *b, int color) {
  ull bits = b->disks[color];
  int ndisks = 0;
  for (; bits; ndisks++) {
    bits &= bits - 1; // clear the least significant bit set
  }
  return ndisks;
}

void EndGame(Board b) {
  int o_score = CountBitsOnBoard(&b, O_WHITE);
  int x_score = CountBitsOnBoard(&b, X_BLACK);
  printf("Game over. \n");
  if (o_score == x_score) {
    printf("Tie game. Each player has %d disks\n", o_score);
  } else {
    printf("X has %d disks. O has %d disks. %c wins.\n", x_score, o_score,
           (x_score > o_score ? 'X' : 'O'));
  }
}