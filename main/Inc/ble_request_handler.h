/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : ble_request_handler.h
 * @brief          : Header for ble_request_handler.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef CONFIG_HANDLE_H
#define CONFIG_HANDLE_H

#include <stdint.h>
#include "uart_handler.h"

int32_t ble_request_handler(const uint8_t *data);

#endif /* END OF CONFIG_HANDLE_H*/