/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : crc.h
 * @brief          : Header for crc.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stdbool.h>

void crc32_init(void);
uint32_t crc32(const uint8_t *data, uint32_t length);
bool validate_crc(const uint8_t* data, uint32_t length, uint32_t crc_in_packet);

#endif /* END OF CRC_H */
