/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : uart_handler.h
 * @brief          : Header for uart_handler.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef UART_HANDLER_H
#define UART_HANDLER_H

/* HEADER FILES */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

/* MACROS */
#define START_BYTE              0xA5
#define PAYLOAD_LEN             (0x100)

/* STRUCTS */
#pragma pack(1)
typedef struct {
    uint8_t start_byte;
    uint8_t packet_type;
    uint32_t length;
    uint8_t payload[PAYLOAD_LEN];
    uint32_t crc;
}uart_data_pack_t;

/* ENUMS */
typedef enum {
    BLE_ACK = 0x01,
    BLE_NACK  = 0x02,
    BLE_START_ADV = 0x10,
    BLE_STOP_ADV = 0x11,
    BLE_CONNECTED  = 0x12,
    BLE_DISCONNECTED = 0x13,
    BLE_FW_UPLOAD_START = 0x20,
    BLE_FW_UPLOAD_DATA = 0x21,
    BLE_FW_UPLOAD_END = 0x22,
    BLE_FW_UPLOAD_SUCCESS = 0x23,
    BLE_FW_UPLOAD_FAIL = 0x24,
    BLE_FW_CHUNK_START = 0x30,
    BLE_FW_CHUNK_DATA = 0x31,
    BLE_FW_CHUNK_END = 0x32,
    BLE_FW_PROGRESS = 0x40,
    BLE_CONFIG_REQUEST = 0x50,
    BLE_CONFIG_RESPONSE = 0x51,
    BLE_VERSION_REQUEST = 0x52,
    BLE_VERSION_RESPONSE = 0x53,
    BLE_PKT_ERROR = 0x60
}packet_type_t;

typedef enum {
    SUCCESS = 0x00,
    START_BYTE_ERROR = -1,
    INVALID_PACKET_ERROR = -2, 
    CRC_CHECK_ERROR = -3,
    UART_TX_ERROR = -4,
    UART_RX_ERROR = -5,
    REQUEST_PACKET_NOT_RECEIVED = -6,
    UNKNOWN_ERROR
}error_code_t;

typedef enum {
    UART_WRITE_START,
    UART_WRITE_DATA,
    UART_WRITE_END
}uart_write_state_t;

/* FUNCTION PROTOTYPES */
void uart_initialization(void);
int uart_data_handler(const uint8_t* data);
uint8_t* uart_read_data(uint32_t* length);
void uart_event_task(void *pvParameters);
void set_connection_handle(uint16_t hndl);
uint16_t get_connection_handle(void);

#endif /* END OF UART_HANDLER_H */