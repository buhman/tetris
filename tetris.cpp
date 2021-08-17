#include <algorithm>
#include <cassert>
#include <chrono>
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
    {{3, 0}, {4, 0}, {4, 1}, {5, 1}},
    {{5, 0}, {4, 1}, {5, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {4, 2}, {5, 2}},
    {{4, 0}, {3, 1}, {4, 1}, {3, 2}}
  },
  [(int)tetris::tet::l] = {
    {{5, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {4, 1}, {4, 2}, {5, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {3, 2}},
    {{3, 0}, {4, 0}, {4, 1}, {4, 2}}
  },
  [(int)tetris::tet::o] = {
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}}
  },
  [(int)tetris::tet::s] = {
    {{4, 0}, {5, 0}, {3, 1}, {4, 1}},
    {{4, 0}, {4, 1}, {5, 1}, {5, 2}},
    {{4, 1}, {5, 1}, {3, 2}, {4, 2}},
    {{3, 0}, {3, 1}, {4, 1}, {4, 2}}
  },
  [(int)tetris::tet::i] = {
    {{3, 1}, {4, 1}, {5, 1}, {6, 1}},
    {{5, 0}, {5, 1}, {5, 2}, {5, 3}},
    {{3, 2}, {4, 2}, {5, 2}, {6, 2}},
    {{4, 0}, {4, 1}, {4, 2}, {4, 3}}
  },
  [(int)tetris::tet::j] = {
    {{3, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {5, 2}},
    {{4, 0}, {4, 1}, {3, 2}, {4, 2}}
  },
  [(int)tetris::tet::t] = {
    {{4, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {4, 1}, {5, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {4, 2}},
    {{4, 0}, {3, 1}, {4, 1}, {4, 2}}
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
  if (p.tet == tetris::tet::i)
    p.pos.v = 19;
  else
    p.pos.v = 20;
}

void tetris::swap()
{
  assert(tetris::this_side != tetris::side_t::none);

  THIS_FRAME.swapped = true;
  tetris::tet swap = THIS_FRAME.swap;
  THIS_FRAME.swap = THIS_FRAME.piece.tet;
  if (THIS_FRAME.swap != tetris::tet::empty)
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
  // bag
  // queue
  // swap
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
    return true;
  }
}

void tetris::init()
{
  for (int i = 0; i < tetris::frame_count; i++) {
  }
}
