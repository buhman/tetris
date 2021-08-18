#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include <random>
#include <set>

#include "tetris.hpp"
#include "client.hpp"

#define THIS_FRAME (tetris::frames[(int)tetris::this_side])

tetris::side_t tetris::this_side = tetris::side_t::none;
std::array<tetris::frame, tetris::frame_count> tetris::frames;

tetris::coord tetris::offsets[(int)tetris::tet::last][4][4] = {
  [(int)tetris::tet::z] = {
    {{3, 3}, {4, 3}, {4, 2}, {5, 2}},
    {{5, 3}, {4, 2}, {5, 2}, {4, 1}},
    {{3, 2}, {4, 2}, {4, 1}, {5, 1}},
    {{4, 3}, {3, 2}, {4, 2}, {3, 1}},
  },
  [(int)tetris::tet::l] = {
    {{5, 3}, {3, 2}, {4, 2}, {5, 2}},
    {{4, 3}, {4, 2}, {4, 1}, {5, 1}},
    {{3, 2}, {4, 2}, {5, 2}, {3, 1}},
    {{3, 3}, {4, 3}, {4, 2}, {4, 1}},
  },
  [(int)tetris::tet::o] = {
    {{4, 3}, {5, 3}, {4, 2}, {5, 2}},
    {{4, 3}, {5, 3}, {4, 2}, {5, 2}},
    {{4, 3}, {5, 3}, {4, 2}, {5, 2}},
    {{4, 3}, {5, 3}, {4, 2}, {5, 2}},
  },
  [(int)tetris::tet::s] = {
    {{4, 3}, {5, 3}, {3, 2}, {4, 2}},
    {{4, 3}, {4, 2}, {5, 2}, {5, 1}},
    {{4, 2}, {5, 2}, {3, 1}, {4, 1}},
    {{3, 3}, {3, 2}, {4, 2}, {4, 1}},
  },
  [(int)tetris::tet::i] = {
    {{3, 2}, {4, 2}, {5, 2}, {6, 2}},
    {{5, 3}, {5, 2}, {5, 1}, {5, 0}},
    {{3, 1}, {4, 1}, {5, 1}, {6, 1}},
    {{4, 3}, {4, 2}, {4, 1}, {4, 0}},
  },
  [(int)tetris::tet::j] = {
    {{3, 3}, {3, 2}, {4, 2}, {5, 2}},
    {{4, 3}, {5, 3}, {4, 2}, {4, 1}},
    {{3, 2}, {4, 2}, {5, 2}, {5, 1}},
    {{4, 3}, {4, 2}, {3, 1}, {4, 1}},
  },
  [(int)tetris::tet::t] = {
    {{4, 3}, {3, 2}, {4, 2}, {5, 2}},
    {{4, 3}, {4, 2}, {5, 2}, {4, 1}},
    {{3, 2}, {4, 2}, {5, 2}, {4, 1}},
    {{4, 3}, {3, 2}, {4, 2}, {4, 1}},
  },
};

static auto seed = std::chrono::system_clock::now().time_since_epoch().count();
static std::default_random_engine generator (seed);

std::uniform_int_distribution<int> tet_distribution(0, (int)tetris::tet::empty - 1);

static void fill(tetris::cell& cell, int i, int j)
{
  cell.color = tetris::tet::empty;
}

static void reset_field(tetris::field& field)
{
  for (int i = 0; i < tetris::columns; i++)
    for (int j = 0; j < tetris::rows; j++)
      fill(field[i][j], i, j);
}

static void refill_bag(tetris::bag& bag)
{
  if (bag.size() == 0) {
    tetris::bag::iterator it = bag.end();
    for (int i = 0; i != (int)tetris::tet::empty; i++) {
      tetris::tet t = static_cast<tetris::tet>(i);
      bag.insert(it, t);
    }
  }
}

static tetris::tet next_tet(tetris::frame& f)
{
  while (f.queue.size() < 6) {
    refill_bag(f.bag);
    std::uniform_int_distribution<int> distribution(0, f.bag.size() - 1);
    int n = distribution(generator);
    tetris::bag::iterator it = f.bag.begin();
    std::advance(it, n);
    f.queue.insert(f.queue.begin(), *it);
    f.bag.erase(it);
  }

  tetris::tet next = f.queue.back();
  f.queue.pop_back();
  return next;
}

static bool collision(tetris::piece& p)
{
  assert(tetris::this_side != tetris::side_t::none);

  for (int i = 0; i < 4; i++) {
    tetris::coord *offset = tetris::offsets[(int)p.tet][(int)p.facing];
    int q = p.pos.u + offset[i].u;
    int r = p.pos.v + offset[i].v;

    tetris::cell cell = THIS_FRAME.field[q][r];
    if (cell.color != tetris::tet::empty || q == -1 || q == 10 || r == -1 || r == 40)
      return true;
  }
  return false;
}

static void update_drop_row(tetris::piece& piece) {
  tetris::piece p = piece;
  while (!collision(p))
    p.pos.v -= 1;
  piece.drop_row = p.pos.v + 1;
}

static void _next_piece(tetris::piece& p, tetris::tet t)
{
  p.tet = t;
  p.pos.u = 0;
  p.pos.v = 18;
  p.facing = tetris::dir::up;
  p.lock_delay.moves = 0;
  p.lock_delay.locking = false;
}

void tetris::swap()
{
  assert(tetris::this_side != tetris::side_t::none);

  THIS_FRAME.swapped = true;
  tetris::tet swap = THIS_FRAME.swap;
  THIS_FRAME.swap = THIS_FRAME.piece.tet;
  if (swap == tetris::tet::empty)
    _next_piece(THIS_FRAME.piece, next_tet(THIS_FRAME));
  else
    _next_piece(THIS_FRAME.piece, swap);
  update_drop_row(THIS_FRAME.piece);
}

void tetris::event_reset_frame(tetris::side_t side)
{
  tetris::frame& frame = frames[(int)side];
  reset_field(frame.field);
  _next_piece(frame.piece, next_tet(frame));
  update_drop_row(frame.piece);
  frame.swap = tetris::tet::empty;
  frame.level = 1;
}


static void clear_lines(tetris::field& field, tetris::piece& piece)
{
  uint64_t seen = 0;
  uint64_t rows = 0;

  for (int i = 0; i < 4; i++) {
    tetris::coord *offset = tetris::offsets[(int)piece.tet][(int)piece.facing];
    int r = piece.pos.v + offset[i].v;
    if ((1L << r) & seen)
      continue;

    seen |= (1L << r);

    for (int x = 0; x < 10; x++) {
      if (field[x][r].color == tetris::tet::empty)
        goto next_line;
    }

    rows |= (1L << r);

  next_line:
    (void)0;
  }

  int off = 0;
  for (int row = 0; row < 40; row++) {
    while (rows & (1L << (row + off))) {
      off++;
    }

    for (int col = 0; col < 10; col++) {
      if ((row + off) >= 40 && off > 0)
        fill(field[col][row], col, row);
      else {
        field[col][row].color = field[col][row + off].color;
      }
    }
  }
}

void tetris::_place(tetris::field& field, tetris::piece& piece)
{
  for (int i = 0; i < 4; i++) {
    tetris::coord *offset = tetris::offsets[(int)piece.tet][(int)piece.facing];
    int q = piece.pos.u + offset[i].u;
    int r = piece.pos.v + offset[i].v;

    tetris::cell& cell = field[q][r];
    assert(cell.color == tetris::tet::empty);
    cell.color = piece.tet;
  }
  clear_lines(field, piece);
}

void tetris::drop()
{
  assert(tetris::this_side != tetris::side_t::none);

  THIS_FRAME.swapped = false;
  THIS_FRAME.piece.pos.v = THIS_FRAME.piece.drop_row;
  _place(THIS_FRAME.field, THIS_FRAME.piece);
}

void tetris::next_piece()
{
  assert(tetris::this_side != tetris::side_t::none);

  _next_piece(THIS_FRAME.piece, next_tet(THIS_FRAME));
  update_drop_row(THIS_FRAME.piece);
}

bool tetris::lock_delay(tetris::piece& piece)
{
  int& moves = piece.lock_delay.moves;
  auto& point = piece.lock_delay.point;
  auto now = tetris::clock::now();

  if (!piece.lock_delay.locking) {
    piece.lock_delay.locking = true;
    point = tetris::clock::now();
    piece.lock_delay.moves = 0;
  }

  float duration = tetris::duration(now - point).count();

  if (moves < 15 && duration < 0.5f) {
    moves++;
    return true;
  } else {
    moves = 0;
    return false;
  }
}

bool tetris::move(tetris::coord offset, int rotation)
{
  assert(tetris::this_side != tetris::side_t::none);

  tetris::piece& piece = THIS_FRAME.piece;
  tetris::piece p = piece;
  p.pos.u += offset.u;
  p.pos.v += offset.v;
  p.facing = static_cast<tetris::dir>(((unsigned int)piece.facing + rotation) % (unsigned int)tetris::dir::last);
  if (collision(p)) {
    return false;
  } else {
    piece.pos.u = p.pos.u;
    piece.pos.v = p.pos.v;
    piece.facing = p.facing;
    if (offset.u || rotation)
      update_drop_row(piece);

    if (piece.lock_delay.locking) {
      piece.lock_delay.moves += 1;
      piece.lock_delay.point = tetris::clock::now();
    }

    return true;
  }
}

static inline float _gravity(int level)
{
  return std::pow((0.8 - ((level - 1) * 0.007)), (level - 1));
}

bool tetris::gravity(tetris::frame& frame)
{
  auto& point = frame.point;
  auto now = tetris::clock::now();

  float duration = tetris::duration(now - point).count();

  if (_gravity(frame.level) < duration) {
    frame.point = tetris::clock::now();
    return true;
  } else {
    return false;
  }
}

void tetris::init()
{
  for (int i = 0; i < tetris::frame_count; i++) {
    tetris::frames[i].queue.clear();
    tetris::frames[i].bag.clear();
    tetris::frames[i].piece.tet = tetris::tet::empty;
    tetris::frames[i].swap = tetris::tet::empty;
    tetris::frames[i].point = tetris::clock::now();
  }
}
