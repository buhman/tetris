#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <cassert>

#include "bswap.hpp"
#include "client.hpp"
#include "message.hpp"
#include "platform_socket.hpp"

#define THIS_FRAME (tetris::frames[static_cast<int>(tetris::this_side)])

struct state {
  int fd;
  std::thread* thread;
};

static state state;

constexpr const char* server_addr = "localhost";
constexpr const char* server_port = "5000";

int reconnect()
{
  int ret;
  int fd = -1;

  if (state.fd >= 0) {
    ret = close(state.fd);
    if (ret < 0)
      std::cerr << "close: " << std::strerror(errno) << '\n';
    state.fd = -1;
  }

  struct addrinfo *result, *rp;
  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  ret = getaddrinfo(server_addr, server_port, &hints, &result);
  if (ret != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(ret) << '\n';
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd < 0) {
      std::cerr << "socket: " << std::strerror(errno) << '\n';
      continue;
    }

    ret = connect(fd, rp->ai_addr, rp->ai_addrlen);
    if (ret == 0) {
      break;
    } else {
      char saddr[INET6_ADDRSTRLEN];
      const char * cret;
      const void * naddr;
      uint16_t nport;
      if (rp->ai_family == AF_INET6) {
        naddr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
        nport = ((struct sockaddr_in6 *)rp->ai_addr)->sin6_port;
      } else if (rp->ai_family == AF_INET) {
        naddr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
        nport = ((struct sockaddr_in *)rp->ai_addr)->sin_port;
      } else
        throw rp->ai_family;

      cret = inet_ntop(rp->ai_family, naddr, saddr, (sizeof (saddr)));
      if (cret == nullptr)
        throw "inet_ntop";

      std::cerr << "connect: " << '[' << saddr << "]:" << bswap::ntoh(nport) << ' ' << std::strerror(errno) << '\n';
      #ifdef _WIN32
      std::cerr << "WSAGetLastError: " << WSAGetLastError() << '\n';
      #endif
      close(fd);
      fd = -1;
    }
  }
  freeaddrinfo(result);

  if (fd != -1) {
    state.fd = fd;
    std::cerr << "connected\n";
    return 0;
  } else {
    return -1;
  }
}

static void send_frame(const message::frame_header_t& header, const message::next_t& next)
{
  assert(state.fd != -1);

  uint8_t buf[4096];
  size_t buf_length = message::encode(header, next, buf);

  ssize_t ret = send(state.fd, buf, buf_length, 0);
  if (ret < 0) {
    std::cerr << "send: " << std::strerror(errno) << '\n';
  }
}

static void event_field(tetris::field& field, tetris::side_t side)
{
  message::frame_header_t header;
  header.type = message::type_t::_field;
  header.side = side;
  header.next_length = message::field::size;

  send_frame(header, message::next_t{field});
}

static void event_next_piece(tetris::piece& piece, tetris::side_t side)
{
  message::frame_header_t header;
  header.type = message::type_t::_next_piece;
  header.side = side;
  header.next_length = message::piece::size;

  send_frame(header, message::next_t{piece});
}

static void event_move(tetris::piece& piece, tetris::side_t side)
{
  message::frame_header_t header;
  header.type = message::type_t::_move;
  header.side = side;
  header.next_length = message::piece::size;

  send_frame(header, message::next_t{piece});
}

static void event_drop(tetris::piece& piece, tetris::side_t side)
{
  message::frame_header_t header;
  header.type = message::type_t::_drop;
  header.side = side;
  header.next_length = message::piece::size;

  send_frame(header, message::next_t{piece});
}

static void loop()
{
  while (1) {
    if (state.fd == -1) {
      while (reconnect() < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      }
    }

    ssize_t ret;
    uint8_t buf_header[message::frame_header::size];
    ret = recv(state.fd, buf_header, message::frame_header::size, 0);
    if (ret <= 0) {
      if (ret < 0) std::cerr << "recv: " << std::strerror(errno) << '\n';
      close(state.fd);
      state.fd = -1;
      continue;
    }
    assert(ret == message::frame_header::size);

    message::frame_header_t header = message::frame_header::decode(buf_header);
    uint8_t buf_frame[header.next_length];
    if (header.next_length != 0) {
      ret = recv(state.fd, buf_frame, header.next_length, 0);
      if (ret <= 0) {
        if (ret < 0) std::cerr << "recv: " << std::strerror(errno) << '\n';
        close(state.fd);
        state.fd = -1;
        continue;
      }
      assert(ret == header.next_length);
    }

    assert(static_cast<int>(header.side) < tetris::frame_count);
    switch (header.type) {
    case message::type_t::_field:
      //std::cerr << "message _field " << (int)header.side << '\n';
      assert(header.side != tetris::this_side);
      assert(header.next_length == message::field::size);
      message::field::decode(buf_frame, tetris::frames[(int)header.side].field);
      break;
    case message::type_t::_side:
      //std::cerr << "message _side " << (int)header.side << '\n';
      assert(header.next_length == 0);
      assert(tetris::this_side == tetris::side_t::none);

      tetris::this_side = header.side;
      tetris::event_reset_frame(header.side);
      event_field(tetris::frames[(int)header.side].field, header.side);
      event_move(tetris::frames[(int)header.side].piece, header.side);

      break;
    case message::type_t::_next_piece:
      assert(header.side != tetris::this_side);
      assert(header.next_length == message::piece::size);
      tetris::_garbage(tetris::frames[(int)header.side].field, tetris::frames[(int)header.side].garbage);
      message::piece::decode(buf_frame, tetris::frames[(int)header.side].piece);
      break;
    case message::type_t::_move:
      //std::cerr << "message _move " << (int)header.side << '\n';
      assert(header.side != tetris::this_side);
      assert(header.next_length == message::piece::size);
      message::piece::decode(buf_frame, tetris::frames[(int)header.side].piece);
      break;
    case message::type_t::_drop:
      //std::cerr << "message _move " << (int)header.side << '\n';
      assert(header.side != tetris::this_side);
      assert(header.next_length == message::piece::size);
      message::piece::decode(buf_frame, tetris::frames[(int)header.side].piece);
      tetris::place(tetris::frames[(int)header.side]);
      break;
    case message::type_t::_attack:
    {
      // no header.side assert
      std::cerr << "recv attack " << (int)header.side << '\n';
      assert(header.next_length == message::attack::size);
      tetris::attack_t attack;
      message::attack::decode(buf_frame, attack);
      tetris::attack(tetris::frames[(int)header.side], attack);
      break;
    }
    default:
      std::cerr << "unhandled frame type " << header.type << '\n';
      break;
    }
  }
}

void client::input(tetris::event ev)
{
  if (tetris::this_side == tetris::side_t::none)
    return;

  switch (ev) {
  case tetris::event::left:
    if (tetris::move({-1, 0}, 0))
      event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::right:
    if (tetris::move({1, 0}, 0))
      event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::down:
    if (tetris::move({0, -1}, 0))
      event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::drop:
    tetris::drop();
    event_drop(THIS_FRAME.piece, tetris::this_side);
    tetris::next_piece();
    event_next_piece(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::spin_cw:
    if (tetris::move({0, 0}, 1))
      event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::spin_ccw:
    if (tetris::move({0, 0}, -1))
      event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::spin_180:
    //if (tetris::move({0, 0}, 2))
    //  event_move(THIS_FRAME.piece, tetris::this_side);
    break;
  case tetris::event::swap:
    if (!THIS_FRAME.swapped) {
      tetris::swap();
      event_move(THIS_FRAME.piece, tetris::this_side);
    }
    break;
  default:
    std::cerr << "unhandled input ev " << static_cast<int>(ev) << '\n';
    break;
  }
}


void client::tick()
{
  if (tetris::this_side == tetris::side_t::none)
    return;

  if (!tetris::gravity(THIS_FRAME))
    return;

  if (tetris::move({0, -1}, 0)) {
    THIS_FRAME.piece.lock_delay.locking = false;
    event_move(THIS_FRAME.piece, tetris::this_side);
  } else {
    if (!tetris::lock_delay(THIS_FRAME.piece)) {
      tetris::drop();
      event_drop(THIS_FRAME.piece, tetris::this_side);
      tetris::_garbage(THIS_FRAME.field, THIS_FRAME.garbage);
      tetris::next_piece();
      event_next_piece(THIS_FRAME.piece, tetris::this_side);
    }
  }
}

void client::init()
{
  #ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup(0x0101, &wsaData))
    throw "WSAStatup";
  #endif

  state.fd = -1;
  state.thread = new std::thread(loop);

  // WSACleanup();
}
