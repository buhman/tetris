#include <cstdint>
#include <cassert>
#include <iostream>

#include "message.hpp"
#include "tetris.hpp"
#include "bswap.hpp"

static inline int _cell_index(int u, int v)
{
  return (v * tetris::columns + u);
}

namespace message {

namespace frame_header {
  frame_header_t decode(const std::uint8_t * buf)
  {
    return {
      static_cast<message::type_t>(*((uint8_t *)(buf + 0))),
      static_cast<tetris::side_t>(*((uint8_t *)(buf + 1))),
      bswap::ntoh(*((uint16_t *)(buf + 2))), // next_length
    };
  }

  void encode(const message::frame_header_t& header, std::uint8_t * buf)
  {
    *((uint8_t *)(buf + 0)) = header.type;
    *((uint8_t *)(buf + 1)) = static_cast<uint8_t>(header.side);
    *((uint16_t *)(buf + 2)) = bswap::hton(header.next_length);
  }
}

// field

namespace field {
  void decode(const std::uint8_t * buf, tetris::field& field)
  {
    for (int u = 0; u < tetris::columns; u++) {
      for (int v = 0; v < tetris::rows; v++) {
        const int bi = _cell_index(u, v);
        field[u][v].color = static_cast<tetris::tet>(buf[bi]);
      }
    }
  }

  void encode(const tetris::field& field, std::uint8_t * buf)
  {
    for (int u = 0; u < tetris::columns; u++) {
      for (int v = 0; v < tetris::rows; v++) {
        const int bi = _cell_index(u, v);
        buf[bi] = static_cast<uint8_t>(field[u][v].color);
      }
    }
  }
}

namespace piece {
  void decode(const std::uint8_t * buf, tetris::piece& piece)
  {
    piece.tet = static_cast<tetris::tet>(buf[0]);
    piece.facing = static_cast<tetris::dir>(buf[1]);
    piece.pos.u = ((std::int8_t*)buf)[2];
    piece.pos.v = ((std::int8_t*)buf)[3];
    piece.drop_row = buf[4];
  }

  void encode(const tetris::piece& piece, std::uint8_t * buf)
  {
    buf[0] = static_cast<uint8_t>(piece.tet);
    buf[1] = static_cast<uint8_t>(piece.facing);
    ((std::int8_t*)buf)[2] = piece.pos.u;
    ((std::int8_t*)buf)[3] = piece.pos.v;
    buf[4] = piece.drop_row;
  }
}

size_t encode(const frame_header_t& header, const next_t& next, std::uint8_t * buf)
{
  message::frame_header::encode(header, buf);
  buf += message::frame_header::size;
  switch (header.type) {
  case message::type_t::_field:
    assert(header.next_length == message::field::size);
    message::field::encode(std::get<tetris::field>(next), buf);
    return message::frame_header::size + message::field::size;
  case message::type_t::_side:
    assert(header.next_length == 0);
    return message::frame_header::size;
  case message::type_t::_move:
  case message::type_t::_drop:
    assert(header.next_length == message::piece::size);
    message::piece::encode(std::get<tetris::piece>(next), buf);
    return message::frame_header::size + message::piece::size;
  default:
    std::cerr << header.type << '\n';
    assert(false);
  }
}

}
