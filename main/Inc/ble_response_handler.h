/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : ble_response_handler.h
 * @brief          : Header for ble_response_handler.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef BLE_RESPONSE_HANDLER_H
#define BLE_RESPONSE_HANDLER_H

void set_data_read_length(uint32_t length);
uint32_t get_data_read_length(void);
void store_config_file(uint8_t* data, uint32_t length);
void reset_buffer_variables(void);
void set_connection_handle(uint16_t hndl);
uint16_t get_connection_handle(void);

#endif /* END OF BLE_RESPONSE_HANDLER_H */