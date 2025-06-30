/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : utils.h
 * @brief          : Header for utils.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint32_t byte_array_to_u32_little_endian(const uint8_t* data);
uint32_t byte_array_to_u32_big_endian(const uint8_t* data);
void u32_to_byte_array_little_endian(uint8_t* data, uint32_t length);
void u32_to_byte_array_big_endian(uint8_t* data, uint32_t length);

#endif /* END OF UTILS_H */