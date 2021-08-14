#include <algorithm>
#include <chrono>
#include <iostream>
#include <queue>
#include <random>
#include <set>

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
  .pos = {0, 0},
  .facing = DIR_0,
};

std::set<tetris::type> bag;
std::vector<tetris::type> tetris::queue;

bool swapped;
tetris::type tetris::swap = TET_LAST;

int tetris::drop_row;

static auto seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator (seed);

void refill_bag() {
  if (bag.size() == 0) {
    std::set<tetris::type>::iterator it = bag.end();
    for (int i = 0; i != TET_LAST; i++) {
      tetris::type t = static_cast<tetris::type>(i);
      bag.insert(it, t);
    }
  }
}

tetris::type next_type() {
  while (queue.size() < 6) {
    refill_bag();
    std::uniform_int_distribution<int> distribution(0, bag.size() - 1);
    int n = distribution(generator);
    std::set<tetris::type>::iterator it = bag.begin();
    std::advance(it, n);
    queue.insert(queue.begin(), *it);
    bag.erase(it);
  }

  /*
  std::vector<tetris::type>::iterator it;
  for (it = queue.begin(); it != queue.end(); it++)
    std::cerr << ' ' << *it << '\n';
  */

  tetris::type next = queue.back();
  queue.pop_back();
  return next;
}

bool collision(tetronimo p) {
  for (int i = 0; i < 4; i++) {
    coord *offset = offsets[p.type][p.facing];
    int q = p.pos.u + offset[i].u;
    int r = p.pos.v + offset[i].v;

    if (r >= 0) {
      tetris::cell cell = tetris::board[q][r];
      if (!cell.empty || r == 20 || q == -1 || q == 10)
        return true;
    }

  }
  return false;
}

void update_drop_row() {
  tetronimo p = piece;
  while (!collision(p))
    p.pos.v += 1;
  tetris::drop_row = p.pos.v - 1;
}

void fall();

void next_piece(tetris::type t) {
  swapped = false;
  if (t == TET_LAST)
    piece.type = next_type();
  else
    piece.type = t;

  piece.pos.u = 0;
  piece.pos.v = -3;
  update_drop_row();
  fall();
}

void fill(int u, int v) {
  //if (u < 3 || u > 6) {
  if (u != 4 && v > 10) {
    board[u][v].empty = false;
    board[u][v].color = TET_LAST;
  } else {
    board[u][v].empty = true;
  }
}

void tetris::initBoard() {
  for (int u = 0; u < 10; u++) {
    for (int v = 0; v < 20; v++) {
      fill(u, v);
    }
  }

  next_piece(TET_LAST);
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

void clear_lines() {
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
        fill(col, row);
      else {
        board[col][row].empty = board[col][row - off].empty;
        board[col][row].color = board[col][row - off].color;
      }
    }
  }
}

void fall() {
  tetronimo p = piece;
  p.pos.v += 1;
  if (collision(p)) {
    if (piece.pos.v <= -2) {
      initBoard();
    } else {
      place();
      clear_lines();
      next_piece(TET_LAST);
    }
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
  case EVENT_SPIN_LEFT:
    p.facing = static_cast<tetris::dir>(((unsigned int)piece.facing + 1) % (unsigned int)DIR_LAST);
    if (!collision(p))
      piece.facing = p.facing;
    break;
  case EVENT_SPIN_RIGHT:
    p.facing = static_cast<tetris::dir>(((unsigned int)piece.facing - 1) % (unsigned int)DIR_LAST);
    if (!collision(p))
      piece.facing = p.facing;
    break;
  case EVENT_SPIN_180:
    p.facing = static_cast<tetris::dir>(((unsigned int)piece.facing + 2) % (unsigned int)DIR_LAST);
    if (!collision(p))
      piece.facing = p.facing;
    break;
  case EVENT_DOWN:
    p.pos.v += 1;
    if (!collision(p))
      piece.pos.v += 1;
    break;
  case EVENT_DROP:
    piece.pos.v = tetris::drop_row;
    fall();
    break;
  case EVENT_SWAP:
  {
    if (!swapped) {
      tetris::type t;
      t = piece.type;
      next_piece(swap);
      swap = t;
      swapped = true;
    }
    break;
  }
  default:
    break;
  }
  update_drop_row();
}

void tetris::tick() {
  fall();
}
