/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : config_file_handle.h
 * @brief          : Header for config_file_handle.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef CONFIG_HANDLE_H
#define CONFIG_HANDLE_H

#include <stdint.h>
#include "uart_handler.h"

uint8_t* get_config_file(uint32_t* length);

#endif /* END OF CONFIG_HANDLE_H*/