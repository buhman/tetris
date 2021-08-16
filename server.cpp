#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bswap.hpp"
#include "message.hpp"
#include "server.hpp"

static int _epoll_fd;
static std::unordered_set<tetris::side_t> sides;

static std::unordered_map<int, poll_action> clients;

//

static void passert(int ret, const char* s)
{
  if (ret < 0) {
    std::cerr << s << ": " << std::strerror(errno) << '\n';
    throw ret;
  }
}

static int open_port(uint16_t port)
{
  int ret;
  int fd;

  ret = ::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  passert(ret, "socket");
  fd = ret;

  int opt = 1;
  ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, (sizeof (int)));
  passert(ret, "setsockopt: SO_REUSEADDR");

  struct sockaddr_in6 sockaddr = {
    .sin6_family = AF_INET6,
    .sin6_port = htons(port),
    .sin6_addr = IN6ADDR_ANY_INIT,
  };

  ret = ::bind(fd, (struct sockaddr *)&sockaddr, (sizeof (struct sockaddr_in6)));
  passert(ret, "bind");

  ret  = ::listen(fd, 1024);
  passert(ret, "listen");

  socklen_t socklen = (sizeof (struct sockaddr_in6));
  ret = ::getsockname(fd, (struct sockaddr *)&sockaddr, &socklen);
  passert(ret, "getsockname");

  std::cerr << "listening on port: " << ntohs(sockaddr.sin6_port) << '\n';

  return fd;
}

static void _epoll_add(const int fd, uint32_t events)
{
  struct epoll_event ev = {
    .events = events,
    .data = { .fd=fd }
  };

  int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  passert(ret, "epoll_ctl: EPOLL_CTL_ADD");
}

static void _epoll_mod(const int fd, uint32_t events)
{
  struct epoll_event ev = {
    .events = EPOLLIN | events | EPOLLONESHOT,
    .data = { .fd=fd }
  };
  int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
  passert(ret, "epoll_ctl: EPOLL_CTL_MOD");
}

static bool handle_send(poll_action& action)
{
  if (action.queue.empty())
    std::cerr << "handle_send " << action.fd << " while action.queue is empty\n";

  while (!action.queue.empty() || action.send.buf_ix) {
    uint16_t buf_len;
    if (action.send.buf_ix) {
      buf_len = action.send.buf_ix;
    } else {
      auto [header, next] = action.queue.front();
      buf_len = message::encode(header, next, action.send.buf);
    }
    ssize_t len = send(action.fd, action.send.buf, buf_len, 0);
    std::cerr << "send: " << len << '\n';
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return false; // keep
      else {
        std::cerr << "recv: " << action.fd << ": " << std::strerror(errno) << '\n';
        return true; // remove
      }
    } else if (len == 0)
      return true; // remove

    if (!action.send.buf_ix)
      action.queue.pop();

    action.send.buf_ix = buf_len - len;
    if (len < buf_len) {
      std::cerr << "send memmove: " << action.send.buf_ix << '\n';
      memmove(action.send.buf, action.send.buf + len, action.send.buf_ix);
    }
  }

  return false; // keep
}

namespace queue_send {
  static void field(poll_action& action, tetris::side_t field_side, tetris::field& field)
  {
    message::frame_header_t header;
    header.type = message::type_t::_field;
    header.side = field_side;
    header.next_length = message::field::size;

    action.queue.push(std::pair{header, message::next_t{field}});
    _epoll_mod(action.fd, EPOLLOUT);
  }

  static void move(poll_action& action, tetris::side_t piece_side, tetris::piece& piece)
  {
    message::frame_header_t header;
    header.type = message::type_t::_move;
    header.side = piece_side;
    header.next_length = message::piece::size;

    action.queue.push(std::pair{header, message::next_t{piece}});
    _epoll_mod(action.fd, EPOLLOUT);
  }

  static void drop(poll_action& action, tetris::side_t piece_side, tetris::piece& piece)
  {
    message::frame_header_t header;
    header.type = message::type_t::_drop;
    header.side = piece_side;
    header.next_length = message::piece::size;

    action.queue.push(std::pair{header, message::next_t{piece}});
    _epoll_mod(action.fd, EPOLLOUT);
  }
}

namespace broadcast {
  static void field(tetris::side_t origin)
  {
    for (auto& client : clients) {
      if (client.second.side == origin || client.second.type == poll_action::accept)
        continue;
      std::cerr << "broadcast field " << static_cast<int>(origin) << " to fd " << client.second.fd << '\n';
      queue_send::field(client.second, origin, tetris::frames[static_cast<int>(origin)].field);
    }
  }

  static void move(tetris::side_t origin)
  {
    for (auto& client : clients) {
      if (client.second.side == origin || client.second.type == poll_action::accept)
        continue;
      std::cerr << "broadcast move " << static_cast<int>(origin) << " to fd " << client.second.fd << '\n';
      queue_send::move(client.second, origin, tetris::frames[static_cast<int>(origin)].piece);
    }
  }

  static void drop(tetris::side_t origin)
  {
    for (auto& client : clients) {
      if (client.second.side == origin || client.second.type == poll_action::accept)
        continue;
      std::cerr << "broadcast drop " << static_cast<int>(origin) << " to fd " << client.second.fd << '\n';
      queue_send::drop(client.second, origin, tetris::frames[static_cast<int>(origin)].piece);
    }
  }
}

namespace dump {
  static void fields(poll_action& action)
  {
    for (int i = 0; i < tetris::frame_count; i++) {
      if (static_cast<int>(action.side) == i)
        continue;
      queue_send::field(action, static_cast<tetris::side_t>(i), tetris::frames[i].field);
    }
  }

  static void moves(poll_action& action)
  {
    for (int i = 0; i < tetris::frame_count; i++) {
      if (static_cast<int>(action.side) == i)
        continue;
      queue_send::move(action, static_cast<tetris::side_t>(i), tetris::frames[i].piece);
    }
  }
}

static size_t handle_recv_frame(poll_action& action, uint8_t *bufi, size_t len)
{
  if (len < message::frame_header::size)
    return 0;

  message::frame_header_t header = message::frame_header::decode(bufi);
  bufi += message::frame_header::size;

  if (len < header.next_length) {
    return 0;
  }

  assert(static_cast<int>(header.side) < tetris::frame_count);
  switch (header.type) {
  case message::type_t::_field:
    assert(header.next_length == message::field::size);
    message::field::decode(bufi, tetris::frames[(int)header.side].field);
    broadcast::field(header.side);
    break;
  case message::type_t::_move:
    assert(header.next_length == message::piece::size);
    message::piece::decode(bufi, tetris::frames[(int)header.side].piece);
    broadcast::move(header.side);
    break;
  case message::type_t::_drop:
    assert(header.next_length == message::piece::size);
    message::piece::decode(bufi, tetris::frames[(int)header.side].piece);
    tetris::_place(tetris::frames[(int)header.side].field, tetris::frames[(int)header.side].piece);
    broadcast::drop(header.side);
    break;
  default:
    std::cerr << "unhandled frame type " << header.type << '\n';
    break;
  }

  return message::frame_header::size + header.next_length;
}

static bool handle_recv(poll_action& action)
{
  while (true) {
    ssize_t len = recv(action.fd, action.recv.buf + action.recv.buf_ix, buf_size - action.recv.buf_ix, 0);
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return false; // keep
      else {
        std::cerr << "recv: " << action.fd << ": " << std::strerror(errno) << '\n';
        return true; // remove
      }
    } else if (len == 0)
      return true; // remove

    uint8_t* bufi = action.recv.buf;
    while (true) {
      size_t offset = handle_recv_frame(action, bufi, len);
      if (offset == 0) {
        break; // bufi does not contain a frame
      } else {
        len -= offset;
        bufi += offset;
      }
    }
    //std::cerr << "recv memmove: " << len << '\n';
    memmove(action.recv.buf, bufi, len);
    action.recv.buf_ix = len;
  }
}

static void allocate_side(poll_action& action)
{
  if (!sides.empty()) {
    action.side = *sides.begin();
    std::cerr << "fd " << action.fd << " side " << static_cast<int>(action.side) << '\n';
    sides.erase(sides.begin());
    message::frame_header_t header{message::type_t::_side, action.side, 0};
    action.queue.push(std::pair{header, message::next_t{}});
  } else
    std::cerr << "no sides remain\n";
}

void foo()
{
  _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  passert(_epoll_fd, "epoll_create1");

  auto listen_fd = open_port(5000);
  auto [it, ok] = clients.try_emplace(listen_fd, listen_fd, poll_action::accept);
  if (!ok) throw "clients.try_emplace";
  _epoll_add(listen_fd, EPOLLIN | EPOLLET);

  std::array<struct epoll_event, 16> events;

  while (1) {
    const int ready_count = epoll_wait(_epoll_fd, events.data(), events.size(), -1);
    if (ready_count < 0 && errno == EINTR)
      continue;
    passert(ready_count, "epoll_wait");

    for (int i = 0; i < ready_count; i++) {
      auto client_it = clients.find(events[i].data.fd);
      if (client_it == clients.end()) throw "clients.find";
      poll_action& action = client_it->second;

      switch (action.type) {
      case poll_action::accept:
        {
          while (1) {
            int accept_fd = accept4(action.fd, NULL, NULL, SOCK_NONBLOCK);
            if (accept_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
              break;
            passert(accept_fd, "accept4");

            auto [accept_it, ok] = clients.try_emplace(accept_fd, accept_fd, poll_action::send_recv);
            if (!ok) throw "clients.try_emplace";
            _epoll_add(accept_fd, EPOLLIN | EPOLLOUT | EPOLLONESHOT);

            // send _side message
            allocate_side(accept_it->second);
            std::cerr << "accept " << accept_fd << " side " << static_cast<int>(accept_it->second.side) << '\n';
            dump::fields(accept_it->second);
            dump::moves(accept_it->second);
          }
        }
        break;
      case poll_action::send_recv:
        {
          bool erase = false;
          if (events[i].events & EPOLLIN)
            erase |= handle_recv(action);
          if (events[i].events & EPOLLOUT)
            erase |= handle_send(action);

          if (erase) {
            int ret = close(action.fd);
            if (ret < 0)
              std::cerr << "close: " << action.fd << ": " << std::strerror(errno) << '\n';
            std::cerr << "clients.erase: " << action.fd << '\n';
            sides.insert(action.side);
            clients.erase(action.fd);
          } else {
            uint32_t epollout = (action.queue.empty() && !action.send.buf_ix) ? 0 : EPOLLOUT;
            //std::cerr << "fd " << action.fd << " epollout " << epollout << '\n';
            _epoll_mod(action.fd, epollout);
          }
        }
        break;
      default:
        throw "unknown type";
        break;
      }
    }
  }

  close(_epoll_fd);
}

int main()
{
  sides.insert(tetris::side_t::zero);
  sides.insert(tetris::side_t::one);
  tetris::init();

  try {
    foo();
  } catch (char const* s) {
    std::cerr << "throw " << s << '\n';
  }

  return 0;
}
