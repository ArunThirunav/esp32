/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : config_file_handle.c
 * @brief          : Handle the configuration file
 ******************************************************************************
 */
/* USER CODE END Header */

#include "config_file_handle.h"
#include "crc.h"
#include "utils.h"
#include <string.h>
#define BLE_MTU (517)
static uint8_t temp[BLE_MTU];

/**
 * @brief Retrieves the configuration file data.
 *
 * This function returns a pointer to the configuration file stored 
 * in memory (e.g., flash, filesystem, or static memory) and provides 
 * its length in bytes. The caller can use this data for parsing or 
 * processing the configuration.
 *
 * @param[out] length   Pointer to a uint32_t variable where the length 
 *                      of the configuration file (in bytes) will be stored.
 *
 * @return              Pointer to the configuration file data buffer.
 *                      Returns NULL if the file is not found or an error occurs.
 */

uint8_t *get_config_file(uint32_t *length)
{
	uint32_t crc_calc;
	temp[0] = START_BYTE;
	temp[1] = BLE_CONFIG_RESPONSE;
	const uint8_t *resp = uart_read_data(length);
	u32_to_byte_array_little_endian(&temp[2], *length);
	memcpy(&temp[6], resp, (*length));
	crc_calc = crc32(temp, (*length) + 6);
	u32_to_byte_array_little_endian(&temp[6 + (*length)], crc_calc);
	*length += 10;
	return &temp[0];
}
