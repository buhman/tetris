#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <span>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bswap.hpp"

//

static void assert(int ret, const char* s)
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
  assert(ret, "socket");
  fd = ret;

  int opt = 1;
  ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, (sizeof (int)));
  assert(ret, "setsockopt: SO_REUSEADDR");

  struct sockaddr_in6 sockaddr = {
    .sin6_family = AF_INET6,
    .sin6_port = htons(port),
    .sin6_addr = IN6ADDR_ANY_INIT,
  };

  ret = ::bind(fd, (struct sockaddr *)&sockaddr, (sizeof (struct sockaddr_in6)));
  assert(ret, "bind");

  ret  = ::listen(fd, 1024);
  assert(ret, "listen");

  socklen_t socklen = (sizeof (struct sockaddr_in6));
  ret = ::getsockname(fd, (struct sockaddr *)&sockaddr, &socklen);
  assert(ret, "getsockname");

  std::cerr << "listening on port: " << ntohs(sockaddr.sin6_port) << '\n';

  return fd;
}

static void epoll_add(const int epoll_fd, uint32_t events, const int fd, const int ix)
{
  struct epoll_event ev = {
    .events = events,
    .data = { .fd=ix }
  };

  int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  assert(ret, "epoll_ctl: EPOLL_CTL_ADD");
}

constexpr unsigned int buf_size = 65536;

struct buf_index
{
  std::array<uint8_t, buf_size> buf;
  std::span<uint8_t> span;
};

struct poll_action
{
  int fd;
  enum action { accept, send_recv } type;
  buf_index send;
  buf_index recv;

  poll_action(const int epoll_fd, const int fd, const action type)
    : fd (fd)
    , type (type)
  {
    recv.span = {recv.buf.begin(), recv.buf.size()};
    send.span = {send.buf.begin(), send.buf.size()};

    switch (type) {
    case accept:
      epoll_add(epoll_fd, EPOLLIN | EPOLLET, fd, fd);
      break;
    case send_recv:
      epoll_add(epoll_fd, EPOLLIN | EPOLLOUT | EPOLLET, fd, fd);
      break;
    }
  }
};

static void handle_send(poll_action& action)
{
}

static void handle_recv(poll_action& action)
{
  while (1) {
    std::cerr << "fd " << action.fd << '\n';
    ssize_t len = recv(action.fd, action.recv.span.data(), action.recv.span.size(), 0);
    if (len < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;
      else
        throw "recv";
    }
    if (len == 0) {
      std::cerr << action.recv.span.size() << ' ';
      throw "eof";
    }

    std::span<uint8_t> span = {action.recv.buf.data(), action.recv.span.data() + len};

    if (span.size() >= sizeof (uint16_t)) {
      uint16_t frame_size = *reinterpret_cast<uint16_t*>(span.data());
      frame_size = bswap::ntoh(frame_size);
      std::cerr << "frame size: " << frame_size << ' ';
      if (span.size() - sizeof (uint16_t) < frame_size) {
        action.recv.span = {action.recv.span.data() + len, action.recv.span.size() - len};
        std::cerr << "partial; rem: " << action.recv.span.size() << '\n';
        continue;
      } else {
        uint32_t f = *reinterpret_cast<uint32_t*>(span.data() + sizeof (uint16_t));
        f = bswap::ntoh(f);
        action.recv.span = {action.recv.buf.begin(), action.recv.buf.size()};
        std::cerr << *reinterpret_cast<float*>(&f) << " complete\n";
      }
    }
  }
}

void foo()
{
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  assert(epoll_fd, "epoll_create1");

  std::unordered_map<int, poll_action> clients;

  auto listen_fd = open_port(5000);
  auto [it, ok] = clients.try_emplace(listen_fd, epoll_fd, listen_fd, poll_action::accept);
  if (!ok) throw "clients.try_emplace";

  std::array<struct epoll_event, 16> events;

  while (1) {
    const int ready_count = epoll_wait(epoll_fd, events.data(), events.size(), -1);
    if (ready_count < 0 && errno == EINTR)
      continue;
    assert(ready_count, "epoll_wait");

    for (int i = 0; i < ready_count; i++) {
      auto client_it = clients.find(events[i].data.fd);

      if (client_it == clients.end()) throw "clients.find";
      switch (client_it->second.type) {
      case poll_action::accept:
        {
          int ret = accept4(client_it->second.fd, NULL, NULL, SOCK_NONBLOCK);
          if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
          assert(ret, "accept4");

          auto [accept_it, ok] = clients.try_emplace(ret, epoll_fd, ret, poll_action::send_recv);
          if (!ok) throw "clients.try_emplace";
        }
        break;
      case poll_action::send_recv:
        {
          handle_recv(client_it->second);
          std::cerr << "sr\n";
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
