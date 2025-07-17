/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : utility.h
 * @brief          : Header for utils.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define BLE_MTU 					(517)
#define START_BYTE_INDEX            (0)
#define PACKET_TYPE_INDEX           (1)
#define PAYLOAD_START_INDEX         (2)
#define CRC_LENGTH                  (4)
#define HEADER_SIZE                 (6) /* START BYTE-1; PACKET TYPE-1; LENGTH-4 */

uint32_t byte_array_to_u32_little_endian(const uint8_t* data);
uint32_t byte_array_to_u32_big_endian(const uint8_t* data);
void u32_to_byte_array_little_endian(uint8_t* data, uint32_t length);
void u32_to_byte_array_big_endian(uint8_t* data, uint32_t length);

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
    BLE_READ_CONFIG_REQUEST = 0x50,
    BLE_READ_CONFIG_RESPONSE = 0x51,
    BLE_VERSION_REQUEST = 0x52,
    BLE_VERSION_RESPONSE = 0x53,
    BLE_WRITE_CONFIG_REQUEST = 0x54,
    BLE_WRITE_CONFIG_RESPONSE = 0x55,
    BLE_PKT_ERROR = 0x60,
    START_BYTE = 0xA5
}packet_type_t;

typedef enum {
    SUCCESS = 0x00,
    START_BYTE_ERROR = -1,
    INVALID_PACKET_ERROR = -2, 
    CRC_CHECK_ERROR = -3,
    UART_TX_ERROR = -4,
    UART_RX_ERROR = -5,
    REQUEST_PACKET_NOT_RECEIVED = -6,
    FILE_CORRUPTION_ERROR = -7,
    UNKNOWN_ERROR
}error_code_t;

#endif /* END OF UTILS_H */