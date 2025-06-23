#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stdbool.h>

uint32_t crc32(const uint8_t *data, uint32_t length);
bool validate_crc(const uint8_t* data, uint32_t length, uint32_t crc_in_packet);

#endif /* END OF CRC_H */
