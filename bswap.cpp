#include <cstdint>

#include "bswap.hpp"

static inline uint16_t bswap_16(uint16_t x)
{
  return x<<8 | x>>8;
}

static inline uint32_t bswap_32(uint32_t x)
{
  return x>>24 | x>>8&0xff00 | x<<8&0xff0000 | x<<24;
}

static inline uint64_t bswap_64(uint64_t x)
{
  return bswap_32(x)+0ULL<<32 | bswap_32(x>>32);
}

namespace bswap {
  uint16_t hton(uint16_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_16(n) : n;
  }

  uint16_t ntoh(uint16_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_16(n) : n;
  }

  uint32_t hton(uint32_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_32(n) : n;
  }

  uint32_t ntoh(uint32_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_32(n) : n;
  }

  uint64_t hton(uint64_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_64(n) : n;
  }

  uint64_t ntoh(uint64_t n)
  {
    union { int i; char c; } u = { 1 };
    return u.c ? bswap_64(n) : n;
  }
}
