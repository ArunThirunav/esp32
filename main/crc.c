#include "crc.h"

/* FUNCTION PROTOTYPE */
static uint32_t reflect(uint32_t data, int bits);
static uint32_t crc32(const uint8_t *data, uint32_t length);

/**
 * @brief
 * @param
 * @param
 * @return
 * */
static uint32_t reflect(uint32_t data, int bits) {
    uint32_t reflection = 0;
    for (int i = 0; i < bits; i++) {
        if (data & (1 << i)) {
            reflection |= (1 << (bits - 1 - i));
        }
    }
    return reflection;
}

static uint32_t crc32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    uint32_t poly = 0x04C11DB7;

    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = reflect(data[i], 8);
        crc ^= ((uint32_t)byte) << 24;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ poly;
            else
                crc <<= 1;
        }
        crc &= 0xFFFFFFFF;
    }

    crc = reflect(crc, 32);
    return crc ^ 0xFFFFFFFF;
}

bool validate_crc(const uint8_t* data){
    uint32_t crc_calc = crc32(data, sizeof(data)-1);
    if (crc_calc == data[sizeof(data)]) {
        return true;
    }
    else {
        return false;
    }
}