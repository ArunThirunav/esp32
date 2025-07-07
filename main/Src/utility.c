/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : utility.c
 * @brief          : Handle the utility functionality like bytearray to u32
 ******************************************************************************
 */
/* USER CODE END Header */

#include "utility.h"
#include "esp_log.h"

/**
 * @brief Converts a 4-byte array in little-endian format to a uint32_t.
 *
 * This function takes 4 consecutive bytes from the input array, 
 * assuming little-endian byte order (LSB first), and converts them 
 * into a 32-bit unsigned integer.
 *
 * @param data    Pointer to a 4-byte array in little-endian format.
 *
 * @return        32-bit unsigned integer converted from the byte array.
 */
uint32_t byte_array_to_u32_little_endian(const uint8_t* data) {    
    return ((uint32_t)data[3]) << 24 |
            ((uint32_t)data[2] << 16) |
            ((uint32_t)data[1] << 8) |
            ((uint32_t)data[0]); 
}

/**
 * @brief Converts a 4-byte array in big-endian format to a uint32_t.
 *
 * This function takes 4 consecutive bytes from the input array, 
 * assuming big-endian byte order (LSB first), and converts them 
 * into a 32-bit unsigned integer.
 *
 * @param data    Pointer to a 4-byte array in big-endian format.
 *
 * @return        32-bit unsigned integer converted from the byte array.
 */
uint32_t byte_array_to_u32_big_endian(const uint8_t* data) {
    return ((uint32_t)data[3])|
            ((uint32_t)data[2] << 8) |
            ((uint32_t)data[1] << 16) |
            ((uint32_t)data[0] << 24); 
}

/**
 * @brief Converts a 32-bit unsigned integer to a 4-byte array in little-endian format.
 *
 * This function converts the input uint32_t value into 4 bytes in little-endian 
 * order (LSB first) and stores them in the provided output array.
 *
 * @param data    Pointer to the output array where the 4 bytes will be stored.
 *                The array must have space for at least 4 bytes.
 * @param value   32-bit unsigned integer to convert.
 *
 * @return        None.
 */
void u32_to_byte_array_little_endian(uint8_t* data, uint32_t value) {
    data[0] = value >> 24;
	data[1] = value >> 16;
	data[2] = value >> 8;
	data[3] = value;  
}

/**
 * @brief Converts a 32-bit unsigned integer to a 4-byte array in big-endian format.
 *
 * This function converts the input uint32_t value into 4 bytes in big-endian 
 * order (LSB first) and stores them in the provided output array.
 *
 * @param data    Pointer to the output array where the 4 bytes will be stored.
 *                The array must have space for at least 4 bytes.
 * @param value   32-bit unsigned integer to convert.
 *
 * @return        None.
 */
void u32_to_byte_array_big_endian(uint8_t* data, uint32_t value) {
    data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
	data[3] = value >> 24;  
}