#pragma once

#include <tuple>

#include "message.hpp"

constexpr unsigned int buf_size = 65536;

struct buf_index
{
  uint8_t buf[buf_size];
  std::size_t buf_ix;
};

using queue_item = std::tuple<message::frame_header_t, message::next_t>;

struct poll_action
{
  int fd;
  enum action { accept, send_recv } type;
  buf_index send;
  buf_index recv;

  std::queue<queue_item> queue;

  poll_action(const int fd, const action type)
    : fd (fd)
    , type (type)
  {
    send.buf_ix = 0;
    recv.buf_ix = 0;
  }
};
