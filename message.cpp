#include <vector>
#include <cassert>
#include <cstdint>
#include <span>

#include "message.hpp"

void decode(std::span<std::uint8_t>& buf, struct message::new_piece& msg)
{
  assert(buf.size() == 1);
  msg.type = static_cast<tetris::type>(buf[0]);
}

void decode(std::span<std::uint8_t>& buf, struct message::piece_move& msg)
{
  assert(buf.size() == 3);
  msg.coord = tetris::coord{
    static_cast<int>(buf[0]),
    static_cast<int>(buf[1]),
  };
  msg.dir = static_cast<tetris::dir>(buf[2]);
}

void decode(std::span<std::uint8_t>& buf, struct message::piece_place& msg)
{
  assert(buf.size() == 3);
  msg.coord = tetris::coord{
    static_cast<int>(buf[0]),
    static_cast<int>(buf[1]),
  };
  msg.dir = static_cast<tetris::dir>(buf[2]);
}

std::uint16_t encode(struct message::new_piece& msg, std::span<std::uint8_t>& buf)
{
  assert(buf.size() >= 1);
  buf[0] = static_cast<uint8_t>(msg.type);

  return 3;
}

std::uint16_t encode(struct message::piece_move& msg, std::span<std::uint8_t>& buf)
{
  assert(buf.size() >= 3);
  buf[0] = static_cast<uint8_t>(msg.coord.u);
  buf[1] = static_cast<uint8_t>(msg.coord.v);
  buf[2] = static_cast<uint8_t>(msg.dir);

  return 3;
}

std::uint16_t encode(struct message::piece_place& msg, std::span<std::uint8_t>& buf)
{
  assert(buf.size() >= 3);
  buf[0] = static_cast<uint8_t>(msg.coord.u);
  buf[1] = static_cast<uint8_t>(msg.coord.v);
  buf[2] = static_cast<uint8_t>(msg.dir);

  return 3;
}
