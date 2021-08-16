#pragma once

#include <variant>

#include "tetris.hpp"

namespace message {
  enum type_t {
    _field,
    _side,
    _move,
    _drop,
  };

  using next_t = std::variant<std::monostate, tetris::field, tetris::piece>;

  // frame_header

  struct frame_header_t {
    type_t type;
    tetris::side_t side;
    uint16_t next_length;
  };

  namespace frame_header {
    frame_header_t decode(const std::uint8_t * buf);
    void encode(const frame_header_t& header, std::uint8_t * buf);

    constexpr uint16_t size = (sizeof (uint8_t))
                            + (sizeof (uint8_t))
                            + (sizeof (uint16_t));
  }

  // field

  namespace field {
    void decode(const std::uint8_t * buf, tetris::field& field);
    void encode(const tetris::field& field, std::uint8_t * buf);

    constexpr uint16_t size = (sizeof (uint8_t)) * tetris::rows * tetris::columns;
  }

  // piece

  namespace piece {
    void decode(const std::uint8_t * buf, tetris::piece& piece);
    void encode(const tetris::piece& piece, std::uint8_t * buf);

    constexpr uint16_t size = (sizeof (uint8_t))     // tet
                            + (sizeof (uint8_t))     // facing
                            + (sizeof (int8_t)) * 2 // pos
                            + (sizeof (uint8_t));    // drop_row
  }

  //

  size_t encode(const frame_header_t& header, const next_t& next, std::uint8_t * buf);
}
