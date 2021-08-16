#include <algorithm>
#include <chrono>
#include <iostream>
#include <queue>
#include <random>
#include <set>

#include "tetris.hpp"

#include "client.hpp"

tetris::side_t tetris::this_side = tetris::side_t::none;
std::array<tetris::frame, tetris::frame_count> tetris::frames;

tetris::coord tetris::offsets[tetris::tet::last][4][4] = {
  [tetris::tet::z] = {
    {{3, 0}, {4, 0}, {4, 1}, {5, 1}},
    {{5, 0}, {4, 1}, {5, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {4, 2}, {5, 2}},
    {{4, 0}, {3, 1}, {4, 1}, {3, 2}}
  },
  [tetris::tet::l] = {
    {{5, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {4, 1}, {4, 2}, {5, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {3, 2}},
    {{3, 0}, {4, 0}, {4, 1}, {4, 2}}
  },
  [tetris::tet::o] = {
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {5, 1}}
  },
  [tetris::tet::s] = {
    {{4, 0}, {5, 0}, {3, 1}, {4, 1}},
    {{4, 0}, {4, 1}, {5, 1}, {5, 2}},
    {{4, 1}, {5, 1}, {3, 2}, {4, 2}},
    {{3, 0}, {3, 1}, {4, 1}, {4, 2}}
  },
  [tetris::tet::i] = {
    {{3, 1}, {4, 1}, {5, 1}, {6, 1}},
    {{5, 0}, {5, 1}, {5, 2}, {5, 3}},
    {{3, 2}, {4, 2}, {5, 2}, {6, 2}},
    {{4, 0}, {4, 1}, {4, 2}, {4, 3}}
  },
  [tetris::tet::j] = {
    {{3, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {5, 0}, {4, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {5, 2}},
    {{4, 0}, {4, 1}, {3, 2}, {4, 2}}
  },
  [tetris::tet::t] = {
    {{4, 0}, {3, 1}, {4, 1}, {5, 1}},
    {{4, 0}, {4, 1}, {5, 1}, {4, 2}},
    {{3, 1}, {4, 1}, {5, 1}, {4, 2}},
    {{4, 0}, {3, 1}, {4, 1}, {4, 2}}
  },
};

static auto seed = std::chrono::system_clock::now().time_since_epoch().count();
static std::default_random_engine generator (seed);

std::uniform_int_distribution<int> tet_distribution(0, static_cast<int>(tetris::tet::empty) - 1);

static void fill(tetris::cell& cell, int i, int j)
{
  if (i < 2 || i > 7) {
    cell.color = static_cast<tetris::tet>(tet_distribution(generator));
  } else {
    cell.color = tetris::tet::empty;
  }
}

static void reset_field(tetris::field& field)
{
  for (int i = 0; i < tetris::columns; i++)
    for (int j = 0; j < tetris::rows; j++)
      fill(field[i][j], i, j);
}

void refill_bag(tetris::bag& bag)
{
  if (bag.size() == 0) {
    tetris::bag::iterator it = bag.end();
    for (int i = 0; i != tetris::tet::empty; i++) {
      tetris::tet t = static_cast<tetris::tet>(i);
      bag.insert(it, t);
    }
  }
}

tetris::tet next_tet(tetris::frame& f)
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

void next_piece(tetris::piece& p, tetris::tet t)
{
  p.tet = t;
  p.pos.u = 0;
  if (p.tet == tetris::tet::i)
    p.pos.v = 19;
  else
    p.pos.v = 20;
}

void tetris::event_reset_frame(tetris::side_t side)
{
  tetris::frame& frame = frames[side];
  reset_field(frame.field);
  next_piece(frame.piece, next_tet(frame));
  // bag
  // queue
  // piece
  // swap
}

void tetris::init()
{
  for (int i = 0; i < tetris::frame_count; i++) {
  }
}
