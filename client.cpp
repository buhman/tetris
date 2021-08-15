#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <cassert>

#ifdef _WIN32
#include <winsock2.h>
#include <in6addr.h>
#include <ws2tcpip.h>
#define close(x) closesocket(x)
#define send(fd, buf, len, flags) send(fd, (const char *)buf, len, flags)
#define recv(fd, buf, len, flags) recv(fd, (char *)buf, len, flags)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "bswap.hpp"
#include "client.hpp"
#include "message.hpp"

struct state {
  int fd;
  std::thread* thread;
};

static state state;

constexpr const char* server_addr = "::1";
constexpr uint16_t server_port = 5000;

int reconnect()
{
  int ret;
  int fd;

  if (state.fd >= 0) {
    ret = close(state.fd);
    if (ret < 0)
      std::cerr << "close: " << std::strerror(errno) << '\n';
    state.fd = -1;
  }

  fd = socket(AF_INET6, SOCK_STREAM, 0);
  if (fd < 0) {
    std::cerr << "socket: " << std::strerror(errno) << '\n';
    throw "socket";
  }

  struct in6_addr addr;
  ret = inet_pton(AF_INET6, server_addr, &addr);
  if (ret != 1)
    throw "inet_pton";

  struct sockaddr_in6 sa = {
    .sin6_family = AF_INET6,
    .sin6_port = bswap::hton(server_port),
    .sin6_addr = addr
  };

  ret = connect(fd, (struct sockaddr*)&sa, (sizeof (struct sockaddr_in6)));
  if (ret < 0) {
    std::cerr << "connect: " << std::strerror(errno) << '\n';
    #ifdef _WIN32
    std::cerr << "WSAGetLastError: " << WSAGetLastError() << '\n';
    #endif
    return -1;
  }

  state.fd = fd;
  return 0;
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

void client::event_field_state(tetris::field& field, message::side_t side)
{
  while (state.fd == -1) {
    std::cerr << "wait fd\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  message::frame_header_t header;
  header.type = message::type_t::_field;
  header.side = side;
  header.next_length = message::field::size;

  send_frame(header, message::next_t{field});
}

static void loop()
{
  while (1) {
    if (state.fd == -1) {
      while (reconnect() < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(4000));

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
    ret = recv(state.fd, buf_frame, header.next_length, 0);
    if (ret <= 0) {
      if (ret < 0) std::cerr << "recv: " << std::strerror(errno) << '\n';
      close(state.fd);
      state.fd = -1;
      continue;
    }
    assert(ret == header.next_length);


    switch (header.type) {
    case message::type_t::_field:
    {
      std::cerr << "field state\n";
      assert(header.next_length == message::field::size);
      message::field::decode(buf_frame, tetris::frames[header.side].field);
      break;
    }
    default:
      std::cerr << "unhandled frame type " << header.type << '\n';
      break;
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
