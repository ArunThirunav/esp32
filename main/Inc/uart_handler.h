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



typedef enum {
    UART_READ_START,
    UART_READ_DATA,
    UART_READ_END
}uart_write_state_t;

/* FUNCTION PROTOTYPES */
void uart_initialization(void);
int ble_request_handler(const uint8_t* data);
uint8_t* uart_read_data(uint32_t* length);
void uart_event_task(void *pvParameters);

int write_data(const void *src, uint32_t size);

#endif /* END OF UART_HANDLER_H */