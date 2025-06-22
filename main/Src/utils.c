#include "utils.h"

uint32_t byte_array_to_u32_little_endian(const uint8_t* data) {
    return ((uint32_t)data[0]) << 24 |
            ((uint32_t)data[1] << 16) |
            ((uint32_t)data[2] << 8) |
            ((uint32_t)data[3]); 
}

uint32_t byte_array_to_u32_big_endian(const uint8_t* data) {
    return ((uint32_t)data[0])|
            ((uint32_t)data[1] << 8) |
            ((uint32_t)data[2] << 16) |
            ((uint32_t)data[3] << 24); 
}