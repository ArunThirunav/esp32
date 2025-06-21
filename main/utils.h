#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint32_t byte_array_to_u32_little_endian(const uint8_t* data);
uint32_t byte_array_to_u32_big_endian(const uint8_t* data);

#endif /* END OF UTILS_H */