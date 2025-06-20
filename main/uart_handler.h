#ifndef UART_HANDLER_H
#define UART_HANDLER_H

/* HEADER FILES */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

/* MACROS */
#define START_BYTE              (0xA5)
#define PAYLOAD_LEN             (0x100)

/* STRUCTS */
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
    START_BYTE_ERROR = 0x01,
    INVALID_PACKET_ERROR = 0x02, 
    CRC_CHECK_ERROR = 0x03,
    UNKNOWN_ERROR
}error_code_t;

/* FUNCTION PROTOTYPES */
void uart_event_task(void *pvParameters);
int parse_data(const uint8_t* data);
void uart_initialization(void);

#endif /* END OF UART_HANDLER_H */