#include <iostream>

#include "tetris.hpp"

using namespace tetris;

cell tetris::board[10][20];

coord tetris::offsets[TET_LAST][4][4] = {
  [TET_Z] = {{{3, 0}, {4, 0}, {4, 1}, {5, 1}},
             {{5, 0}, {4, 1}, {5, 1}, {4, 2}},
             {{3, 1}, {4, 1}, {4, 2}, {5, 2}},
             {{4, 0}, {3, 1}, {4, 1}, {3, 2}}},
  [TET_L] = {{{5, 0}, {3, 1}, {4, 1}, {5, 1}},
             {{4, 0}, {4, 1}, {4, 2}, {5, 2}},
             {{3, 1}, {4, 1}, {5, 1}, {3, 2}},
             {{3, 0}, {4, 0}, {4, 1}, {4, 2}}},
  [TET_O] = {{{4, 1}, {5, 1}, {4, 2}, {5, 2}},
             {{4, 1}, {5, 1}, {4, 2}, {5, 2}},
             {{4, 1}, {5, 1}, {4, 2}, {5, 2}},
             {{4, 1}, {5, 1}, {4, 2}, {5, 2}}},
  [TET_S] = {{{4, 0}, {5, 0}, {3, 1}, {4, 1}},
             {{4, 0}, {4, 1}, {5, 1}, {5, 2}},
             {{4, 1}, {5, 1}, {3, 2}, {4, 2}},
             {{3, 0}, {3, 1}, {4, 1}, {4, 2}}},
  [TET_I] = {{{3, 1}, {4, 1}, {5, 1}, {6, 1}},
             {{5, 0}, {5, 1}, {5, 2}, {5, 3}},
             {{3, 2}, {4, 2}, {5, 2}, {6, 2}},
             {{4, 0}, {4, 1}, {4, 2}, {4, 3}}},
  [TET_J] = {{{3, 0}, {3, 1}, {4, 1}, {5, 1}},
             {{4, 0}, {5, 0}, {4, 1}, {4, 2}},
             {{3, 1}, {4, 1}, {5, 1}, {5, 2}},
             {{4, 0}, {4, 1}, {3, 2}, {4, 2}}},
  [TET_T] = {{{4, 0}, {3, 1}, {4, 1}, {5, 1}},
             {{4, 0}, {4, 1}, {5, 1}, {4, 2}},
             {{3, 1}, {4, 1}, {5, 1}, {4, 2}},
             {{4, 0}, {3, 1}, {4, 1}, {4, 2}}},
};

tetronimo tetris::piece = {
  .type = TET_I,
  .pos = {0, 0},
  .facing = DIR_0,
};

void tetris::initBoard() {
  for (int u = 0; u < 10; u++) {
    for (int v = 0; v < 20; v++) {
      board[u][v].empty = true;
    }
  }
}

void tetris::shiftDown() {
  for (int u = 9; u >= 0; u--) {
    for (int v = 19; v >= 0; v--) {
      if (v == 0)
        board[u][v].empty = true;
      else {
        board[u][v].color = board[u][v-1].color;
        board[u][v].empty = board[u][v-1].empty;
      }
    }
  }
}

bool collision(tetronimo p) {
  for (int i = 0; i < 4; i++) {
    coord *offset = offsets[p.type][p.facing];
    int q = p.pos.u + offset[i].u;
    int r = p.pos.v + offset[i].v;

    tetris::cell cell = tetris::board[q][r];
    if (!cell.empty || r == 20 || q == -1 || q == 10)
      return true;

  }
  return false;
}

void place() {
  for (int i = 0; i < 4; i++) {
    coord *offset = offsets[piece.type][piece.facing];
    int q = piece.pos.u + offset[i].u;
    int r = piece.pos.v + offset[i].v;

    tetris::cell* cell = &tetris::board[q][r];
    cell->empty = false;
    cell->color = piece.type;
  }
}

void clearLines() {
  uint32_t seen = 0;
  uint32_t rows = 0;

  for (int i = 0; i < 4; i++) {
    coord *offset = offsets[piece.type][piece.facing];
    int r = piece.pos.v + offset[i].v;
    if ((1 << r) & seen)
      continue;

    seen |= (1 << r);

    for (int x = 0; x < 10; x++) {
      if (board[x][r].empty)
        goto next_line;
    }

    rows |= (1 << r);

  next_line:
    (void)0;
  }

  int off = 0;
  for (int row = 19; row >= 0; row--) {
    while (rows & (1 << (row - off)))
      off++;

    for (int col = 0; col < 10; col++) {
      if ((row - off) <= 0 && off > 0)
        board[col][row].empty = true;
      else {
        board[col][row].empty = board[col][row - off].empty;
        board[col][row].color = board[col][row - off].color;
      }
    }
  }
}

void drop() {
  tetronimo p = piece;
  p.pos.v += 1;
  if (collision(p)) {
    place();
    clearLines();
    piece.type = static_cast<type>(((int)piece.type + 1) % ((int)TET_LAST));
    piece.pos.u = -1;
    piece.pos.v = 0;
  } else
    piece.pos.v += 1;
}

void tetris::input(event ev) {
  tetronimo p = piece;
  switch (ev) {
  case EVENT_LEFT:
    p.pos.u -= 1;
    if (!collision(p))
      piece.pos.u -= 1;
    break;
  case EVENT_RIGHT:
    p.pos.u += 1;
    if (!collision(p))
      piece.pos.u += 1;
    break;
  case EVENT_SPIN:
    p.facing = static_cast<tetris::dir>((piece.facing + 1) % (int)DIR_LAST);
    if (!collision(p))
      piece.facing = p.facing;
    break;
  case EVENT_DOWN:
    p.pos.v += 1;
    if (!collision(p))
      piece.pos.v += 1;
    break;
  case EVENT_DROP:
    while (!collision(p))
      p.pos.v += 1;
    piece.pos.v = p.pos.v - 1;
    break;
  }
}

void tetris::tick() {
  //shiftDown();
  //
  drop();
}
