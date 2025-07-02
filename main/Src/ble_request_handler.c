/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : ble_request_handler.c
 * @brief          : Handle the configuration file
 ******************************************************************************
 */
/* USER CODE END Header */

#include <string.h>
#include "ble_request_handler.h"
#include "crc.h"
#include "utility.h"
#include "esp_log.h"

/* DEFINES */

/* VARIABLES */

/* FUNCTION PROTOTYPES */

/**
 * @brief Handles incoming data to send to UART.
 *
 * This function processes the received UART data buffer, parses commands, 
 * verifies packet formats, or forwards the data for further handling. 
 * It is typically invoked from the UART event task or a UART ISR context.
 *
 * @param data   Pointer to the UART data buffer.
 *
 * @return       0 if data is handled successfully,
 *               non-zero error code if parsing or validation fails.
 */
int32_t ble_request_handler(const uint8_t *data)
{
    error_code_t status = ESP_OK;
    uart_data_pack_t packet;

    packet.start_byte = data[START_BYTE_INDEX];
    packet.packet_type = data[PACKET_TYPE_INDEX];
    packet.length = byte_array_to_u32_little_endian(&data[PAYLOAD_START_INDEX]);

    /* 6 is the 1(start byte)+1(packet_type)+4(length) */
    packet.crc = byte_array_to_u32_big_endian(&data[HEADER_SIZE + packet.length]);

    /* TODO: FOR FUTURE USE. FOLLOWING COPY IS DONE */
    if (packet.length != 0 && packet.length <= PAYLOAD_LEN)
    {
        memcpy(packet.payload, &data[3], packet.length);
    }

    status = validate_crc(data, (6 + packet.length), packet.crc);
    if (true != status)
    {
        return CRC_CHECK_ERROR;
    }

    ESP_LOGI("PACKET", "start_byte: 0x%X", packet.start_byte);
    ESP_LOGI("PACKET", "packet_type: 0x%X", packet.packet_type);
    ESP_LOGI("PACKET", "length: 0x%lX", packet.length);
    ESP_LOGI("PACKET", "crc: 0x%lX", packet.crc);

    switch (packet.packet_type)
    {
    case BLE_CONFIG_REQUEST:
        ESP_LOGI("BLE_CONFIG_REQUEST", "Received: BLE_CONFIG_REQUEST");
        status = write_data(data, (HEADER_SIZE + CRC_LENGTH + packet.length));
        break;
    case BLE_VERSION_REQUEST:
        ESP_LOGI("BLE_VERSION_REQUEST", "Received: BLE_VERSION_REQUEST");
        /* TODO: RIGHT NOW HARDCODED. NEED TO BE UNCOMMENTED WHEN VERSION REQUEST FUNCTIONALITY IMPLEMENTED */
        // status = uart_write_bytes(UART_PORT, data, (10 + packet.length));
        break;
    default:
        ESP_LOGI("PACKET ERROR", "Received: INVALID REQUEST %d", packet.packet_type);
        status = INVALID_PACKET_ERROR;
        break;
    }
    if (status > 0)
    {
        status = BLE_ACK;
    }
    return status;
}

