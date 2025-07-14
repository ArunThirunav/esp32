/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : crc.c
 * @brief          : Handle the crc functionality
 ******************************************************************************
 */
/* USER CODE END Header */

#include "crc.h"
#include "esp_log.h"

/* DEFINES */
#define CRC_MASK            (0xFFFFFFFF)
#define POLYNOMIAL          (0x04C11DB7)

/* VARIABLES */

/* FUNCTION PROTOTYPE */
static uint32_t reflect(uint32_t data, uint8_t bits);

/**
 * @brief Reflects (reverses) the lower 'bits' number of bits in the given data.
 *
 * This function takes the lower 'bits' bits from the input 'data' and reverses their order.
 * It is commonly used in CRC calculations where input or output reflection is required.
 *
 * @param data  The input data whose bits are to be reflected.
 * @param bits  The number of least significant bits to reflect (e.g., 8, 16, 32).
 *
 * @return      The reflected value of the input data over the specified number of bits.
 */
static uint32_t reflect(uint32_t data, uint8_t bits) {
    uint32_t reflection = 0;
    for (uint8_t i = 0; i < bits; i++) {
        if (data & (1 << i)) {
            reflection |= (1 << (bits - 1 - i));
        }
    }
    return reflection;
}

/**
 * @brief Calculates CRC-32 checksum for the given input data buffer.
 *
 * This function computes the CRC-32 checksum using the standard polynomial 
 * (0x04C11DB7) with input reflection, output reflection, and final XOR, 
 * commonly used in Ethernet, ZIP, and other protocols.
 *
 * @param data    Pointer to the input data buffer.
 * @param length  Length of the data buffer in bytes.
 *
 * @return        Computed CRC-32 value as a 32-bit unsigned integer.
 */
uint32_t crc32(const uint8_t *data, uint32_t length) {
    uint32_t crc = CRC_MASK;
    uint32_t poly = POLYNOMIAL;
    
    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = reflect(data[i], 8);
        crc ^= ((uint32_t)byte) << 24;

        for (uint8_t j = 0; j < 8; j++) {
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

/**
 * @brief Validates the CRC-32 of a given data buffer against an expected CRC value.
 *
 * This function computes the CRC-32 of the provided data buffer and compares 
 * it with the CRC value received (typically included in a packet). It checks 
 * data integrity by ensuring the computed CRC matches the provided one.
 *
 * @param data            Pointer to the input data buffer (excluding the CRC itself).
 * @param length          Length of the data buffer in bytes.
 * @param crc_in_packet   The expected CRC-32 value received in the packet.
 *
 * @return                true if the computed CRC matches the provided CRC, 
 *                        false otherwise.
 */
bool validate_crc(const uint8_t* data, uint32_t length, uint32_t crc_in_packet){
    uint32_t crc_calc = crc32(data, length);

    if (crc_calc == crc_in_packet) {
        return true;
    }
    else {
        ESP_LOGI("CRC CALCULATED", "0x%X", (int)crc_calc);
        ESP_LOGI("CRC PACKET", "0x%X", (int)crc_in_packet);
        ESP_LOGI("CRC", "FAIL");
        return false;
    }
}