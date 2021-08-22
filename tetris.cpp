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

const tetris::coord tetris::offsets[(int)tetris::tet::last][4][4] = {
  [(int)tetris::tet::z] = {
    {{ 0, 0}, { 1, 0}, { 0, 1}, {-1, 1}},
    {{ 0, 0}, { 0,-1}, { 1, 0}, { 1, 1}},
    {{ 0, 0}, { 0,-1}, { 1,-1}, {-1, 0}},
    {{ 0, 0}, {-1, 0}, {-1,-1}, { 0, 1}},
  },
  [(int)tetris::tet::l] = {
    {{ 0, 0}, {-1, 0}, { 1, 0}, { 1, 1}},
    {{ 0, 0}, { 0,-1}, { 1,-1}, { 0, 1}},
    {{ 0, 0}, {-1, 0}, { 1, 0}, {-1,-1}},
    {{ 0, 0}, { 0,-1}, { 0, 1}, {-1, 1}},
  },
  [(int)tetris::tet::o] = {
    {{ 0, 0}, { 0, 1}, { 1, 0}, { 1, 1}},
    {{ 0, 0}, { 0,-1}, { 1, 0}, { 1,-1}},
    {{ 0, 0}, { 0,-1}, {-1, 0}, {-1,-1}},
    {{ 0, 0}, { 0, 1}, {-1, 0}, {-1, 1}},
  },
  [(int)tetris::tet::s] = {
    {{ 0, 0}, {-1, 0}, { 0, 1}, { 1, 1}},
    {{ 0, 0}, { 0, 1}, { 1, 0}, { 1,-1}},
    {{ 0, 0}, { 1, 0}, { 0,-1}, {-1,-1}},
    {{ 0, 0}, { 0,-1}, {-1, 0}, {-1, 1}},
  },
  [(int)tetris::tet::i] = {
    {{ 0, 0}, {-1, 0}, { 1, 0}, { 2, 0}},
    {{ 0, 0}, { 0, 1}, { 0,-1}, { 0,-2}},
    {{ 0, 0}, { 1, 0}, {-1, 0}, {-2, 0}},
    {{ 0, 0}, { 0,-1}, { 0, 1}, { 0, 2}},
  },
  [(int)tetris::tet::j] = {
    {{ 0, 0}, { 1, 0}, {-1, 0}, {-1, 1}},
    {{ 0, 0}, { 0,-1}, { 0, 1}, { 1, 1}},
    {{ 0, 0}, {-1, 0}, { 1, 0}, { 1,-1}},
    {{ 0, 0}, { 0, 1}, { 0,-1}, {-1,-1}},
  },
  [(int)tetris::tet::t] = {
    {{ 0, 0}, {-1, 0}, { 1, 0}, { 0, 1}},
    {{ 0, 0}, { 0, 1}, { 1, 0}, { 0,-1}},
    {{ 0, 0}, { 1, 0}, { 0,-1}, {-1, 0}},
    {{ 0, 0}, {-1, 0}, { 0,-1}, { 0, 1}},
  },
};

using kick_table_t = std::array<std::array<tetris::coord, tetris::kicks>, (int)tetris::dir::last>;

static const kick_table_t zlsjt_kick = {{
  {{{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}}}, // up
  {{{ 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2}}}, // right
  {{{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}}}, // down
  {{{ 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2}}}, // left
}};

static const kick_table_t i_kick = {{
  {{{ 0, 0}, {-1, 0}, { 2, 0}, {-1, 0}, { 2, 0}}},
  {{{-1, 0}, { 0, 0}, { 0, 0}, { 0, 1}, { 0,-2}}},
  {{{-1, 1}, { 1, 1}, {-2, 1}, { 1, 0}, {-2, 0}}},
  {{{ 0, 1}, { 0, 1}, { 0, 1}, { 0,-1}, { 0, 2}}},
}};

static const kick_table_t o_kick = {{
  {{{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}}},
  {{{ 0,-1}, { 0,-1}, { 0,-1}, { 0,-1}, { 0,-1}}},
  {{{-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}}},
  {{{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
}};

static const kick_table_t& kick_offsets(tetris::tet t)
{
  switch (t) {
  case tetris::tet::z:
  case tetris::tet::l:
  case tetris::tet::s:
  case tetris::tet::j:
  case tetris::tet::t:
    return zlsjt_kick;
  case tetris::tet::i:
    return i_kick;
  case tetris::tet::o:
    return o_kick;
  default:
    assert(false);
  }
}

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

  const tetris::coord * offset = tetris::offsets[(int)p.tet][(int)p.facing];

  for (int i = 0; i < 4; i++) {
    int q = p.pos.u + offset[i].u;
    int r = p.pos.v + offset[i].v;

    tetris::cell cell = THIS_FRAME.field[q][r];
    if (cell.color != tetris::tet::empty || q == -1 || q == tetris::columns || r == -1 || r == tetris::rows)
      return true;
  }
  return false;
}

static void update_drop_row(tetris::piece& piece) {
  tetris::piece p = piece;
  assert(!collision(piece));
  while (!collision(p))
    p.pos.v -= 1;
  piece.drop_row = p.pos.v + 1;
}

static void _next_piece(tetris::piece& p, tetris::tet t)
{
  p.tet = t;
  p.pos.u = 4;
  p.pos.v = 20;
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


static int clear_lines(tetris::field& field, tetris::piece& piece)
{
  uint64_t seen = 0;
  uint64_t rows = 0;
  int cleared = 0;

  for (int i = 0; i < 4; i++) {
    const tetris::coord * offset = tetris::offsets[(int)piece.tet][(int)piece.facing];
    int r = piece.pos.v + offset[i].v;
    if ((1L << r) & seen)
      continue;

    seen |= (1L << r);

    for (int x = 0; x < tetris::columns; x++) {
      if (field[x][r].color == tetris::tet::empty)
        goto next_line;
    }

    cleared += 1;
    rows |= (1L << r);

  next_line:
    (void)0;
  }

  int off = 0;
  for (int row = 0; row < tetris::rows; row++) {
    while (rows & (1L << (row + off))) {
      off++;
    }

    for (int col = 0; col < tetris::columns; col++) {
      if ((row + off) >= tetris::rows && off > 0)
        fill(field[col][row], col, row);
      else {
        field[col][row].color = field[col][row + off].color;
      }
    }
  }

  return cleared;
}

namespace points {
  static inline int next_level(int level)
  {
    constexpr int per_level = 10 >> 1;

    return per_level * std::pow(level, 2) + per_level * level;
  }

  static inline int line_clear(int cleared)
  {
    switch (cleared) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 3;
    case 3: return 5;
    case 4: return 8;
    default:
      assert(false);
    }
  }
}

static int _place(tetris::field& field, tetris::piece& piece)
{
  for (int i = 0; i < 4; i++) {
    const tetris::coord * offset = tetris::offsets[(int)piece.tet][(int)piece.facing];
    int q = piece.pos.u + offset[i].u;
    int r = piece.pos.v + offset[i].v;

    tetris::cell& cell = field[q][r];
    assert(cell.color == tetris::tet::empty);
    cell.color = piece.tet;
  }
  int cleared = clear_lines(field, piece);
  return cleared;
}

int tetris::place(tetris::frame& frame) {
  int cleared = _place(frame.field, frame.piece);
  int points = points::line_clear(cleared);
  frame.points += points;
  if (frame.points > points::next_level(frame.level)) {
    frame.level++;
    std::cerr << "level " << frame.level << '\n';
  }
  return cleared;
}

void tetris::drop()
{
  assert(tetris::this_side != tetris::side_t::none);

  THIS_FRAME.swapped = false;
  THIS_FRAME.piece.pos.v = THIS_FRAME.piece.drop_row;
  tetris::place(THIS_FRAME);
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
  p.facing = (tetris::dir)(((unsigned int)piece.facing + rotation) % (unsigned int)tetris::dir::last);

  assert(rotation == 1 || rotation == -1 || rotation == 0);
  p.pos.u = piece.pos.u + offset.u;
  p.pos.v = piece.pos.v + offset.v;

  for (int kick = 0; kick < tetris::kicks; kick++) {
    p.pos.u = piece.pos.u + offset.u;
    p.pos.v = piece.pos.v + offset.v;
    if (rotation != 0) {
      tetris::coord k_offset_a = kick_offsets(piece.tet)[(int)piece.facing][kick];
      tetris::coord k_offset_b = kick_offsets(piece.tet)[(int)p.facing][kick];
      int kick_u = k_offset_a.u - k_offset_b.u;
      int kick_v = k_offset_a.v - k_offset_b.v;
      std::cerr << "kick " << (int)piece.facing << (int)p.facing << " : " << kick_u << ' ' << kick_v << '\n';
      p.pos.u += kick_u;
      p.pos.v += kick_v;
    }

    if (collision(p)) {
      continue;
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
  return false;
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

void __garbage(tetris::field& field, tetris::attack_t& attack)
{
  assert(attack.rows > 0);

  std::cerr << "_garbage processing\n";

  for (int row = tetris::rows - 1; row >= 0; row--) {
    for (int col = 0; col < tetris::columns; col++) {
      if (row < attack.rows) {
        if (col == attack.column)
          field[col][row].color = tetris::tet::empty;
        else
          field[col][row].color = tetris::tet::last;
      } else {
        assert(row - attack.rows >= 0);
        field[col][row].color = field[col][row - attack.rows].color;
      }
    }
  }
}

void tetris::_garbage(tetris::field& field, tetris::garbage_t& garbage)
{
  for (auto& attack : garbage.attacks)
    __garbage(field, attack);
  garbage.attacks.clear();
  garbage.total = 0;
}

void tetris::attack(tetris::frame& frame, tetris::attack_t& attack)
{
  std::cerr << "received attack " << (void*)&frame << ' ' << attack.rows << '\n';
  frame.garbage.total += attack.rows;
  frame.garbage.attacks.push_back(attack);
}

void tetris::init()
{
  for (int i = 0; i < tetris::frame_count; i++) {
    tetris::frames[i].queue.clear();
    tetris::frames[i].bag.clear();
    tetris::frames[i].piece.tet = tetris::tet::empty;
    std::cerr << "dr" << tetris::frames[i].piece.drop_row << '\n';
    tetris::frames[i].swap = tetris::tet::empty;
    tetris::frames[i].point = tetris::clock::now();
  }
}
