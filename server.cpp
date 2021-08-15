#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <queue>
#include <cassert>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bswap.hpp"
#include "server.hpp"
#include "message.hpp"

static std::array<tetris::frame, tetris::frame_count> frames;

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

static void epoll_add(const int epoll_fd, const int fd, uint32_t events)
{
  struct epoll_event ev = {
    .events = events,
    .data = { .fd=fd }
  };

  int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  passert(ret, "epoll_ctl: EPOLL_CTL_ADD");
}

static bool handle_send(poll_action& action)
{
  std::cerr << "handle_send\n";

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

static void handle_recv_message(poll_action& action, message::frame_header_t& header, tetris::field& field)
{
  action.queue.push(std::pair{header, message::next_t{field}});
}

static size_t handle_recv_frame(poll_action& action, uint8_t *bufi, size_t len)
{
  if (len < message::frame_header::size)
    return 0;

  message::frame_header_t header = message::frame_header::decode(bufi);
  bufi += message::frame_header::size;

  std::cerr << "next_length: " << header.next_length;
  if (len < header.next_length) {
    std::cerr << " incomplete\n";
    return 0;
  }
  std::cerr << " complete\n";

  switch (header.type) {
  case message::type_t::_field:
    assert(header.next_length == message::field::size);
    assert(header.side < tetris::frame_count);

    message::field::decode(bufi, frames[header.side].field);
    handle_recv_message(action, header, frames[header.side].field);
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
    std::cerr << "recv memmove: " << len << '\n';
    memmove(action.recv.buf, bufi, len);
    action.recv.buf_ix = len;
  }
}

void foo()
{
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  passert(epoll_fd, "epoll_create1");

  auto listen_fd = open_port(5000);
  auto [it, ok] = clients.try_emplace(listen_fd, listen_fd, poll_action::accept);
  if (!ok) throw "clients.try_emplace";
  epoll_add(epoll_fd, listen_fd, EPOLLIN | EPOLLET);

  std::array<struct epoll_event, 16> events;

  while (1) {
    const int ready_count = epoll_wait(epoll_fd, events.data(), events.size(), -1);
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
          int accept_fd = accept4(action.fd, NULL, NULL, SOCK_NONBLOCK);
          if (accept_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
          passert(accept_fd, "accept4");

          auto [_accept_it, ok] = clients.try_emplace(accept_fd, accept_fd, poll_action::send_recv);
          if (!ok) throw "clients.try_emplace";
          epoll_add(epoll_fd, accept_fd, EPOLLIN | EPOLLONESHOT);
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
            clients.erase(action.fd);
          } else {
            uint32_t epollout = (action.queue.empty() && !action.send.buf_ix ) ? 0 : EPOLLOUT;
            std::cerr << "epo " << epollout << '\n';
            struct epoll_event ev = {
              .events = EPOLLIN | epollout | EPOLLONESHOT,
              .data = { .fd=action.fd }
            };
            int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, action.fd, &ev);
            passert(ret, "epoll_ctl: EPOLL_CTL_MOD");
          }
        }
        break;
      default:
        throw "unknown type";
        break;
      }
    }
  }

  close(epoll_fd);
}

int main()
{
  try {
    foo();
  } catch (char const* s) {
    std::cerr << "throw " << s << '\n';
  }

  return 0;
}
