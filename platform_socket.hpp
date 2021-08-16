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
