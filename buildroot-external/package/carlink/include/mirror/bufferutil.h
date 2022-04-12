#ifndef BUFFERUTIL_H
#define BUFFERUTIL_H
#include <stdbool.h>
#include <stdint.h>

class BufferUtil
{
public:
    static void write32(uint8_t *buffer, unsigned int value);
    static void write16(uint8_t *buffer, unsigned int value);
    static unsigned short read16(const uint8_t *buf);
    static unsigned int read32(const uint8_t *buf);
    static unsigned long long read64(const uint8_t *buf);
};

#endif // BUFFERUTIL_H
