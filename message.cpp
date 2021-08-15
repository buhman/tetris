#include <cstdint>
#include <cassert>

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
      static_cast<message::side_t>(*((uint8_t *)(buf + 1))),
      bswap::ntoh(*((uint16_t *)(buf + 2))), // next_length
    };
  }

  void encode(const message::frame_header_t& header, std::uint8_t * buf)
  {
    *((uint8_t *)(buf + 0)) = header.type;
    *((uint8_t *)(buf + 1)) = header.side;
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

size_t encode(const frame_header_t& header, const next_t& next, std::uint8_t * buf)
{
  message::frame_header::encode(header, buf);
  buf += message::frame_header::size;
  switch (header.type) {
  case message::type_t::_field:
    assert(header.next_length == message::field::size);
    message::field::encode(std::get<tetris::field>(next), buf);
    return message::frame_header::size + message::field::size;
  default:
    assert(false);
  }
}

}

/*
std::uint16_t encode(struct message::new_piece& msg, std::uint8_t* buf)
{
  buf[0] = static_cast<uint8_t>(msg.type);

  return 1;
}

std::uint16_t encode(struct message::piece_move& msg, std::uint8_t* buf)
{
  buf[0] = static_cast<uint8_t>(msg.coord.u);
  buf[1] = static_cast<uint8_t>(msg.coord.v);
  buf[2] = static_cast<uint8_t>(msg.dir);

  return 3;
}

std::uint16_t encode(struct message::piece_place& msg, std::uint8_t* buf)
{
  buf[0] = static_cast<uint8_t>(msg.coord.u);
  buf[1] = static_cast<uint8_t>(msg.coord.v);
  buf[2] = static_cast<uint8_t>(msg.dir);

  return 3;
}

void decode(std::uint8_t* buf, struct message::new_piece& msg)
{
  msg.type = static_cast<tetris::tet>(buf[0]);
}

void decode(std::uint8_t* buf, struct message::piece_move& msg)
{
  msg.coord = tetris::coord{
    static_cast<int>(buf[0]),
    static_cast<int>(buf[1]),
  };
  msg.dir = static_cast<tetris::dir>(buf[2]);
}

void decode(std::uint8_t* buf, struct message::piece_place& msg)
{
  msg.coord = tetris::coord{
    static_cast<int>(buf[0]),
    static_cast<int>(buf[1]),
  };
  msg.dir = static_cast<tetris::dir>(buf[2]);
}
*/
