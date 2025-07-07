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
#define BUF_SIZE (1024)
#define FW_FILE_CHUNK (200)
#define BUFFER_SIZE (FW_FILE_CHUNK * BUF_SIZE) // 256KB
#define CHUNK_SIZE (500)
/* VARIABLES */
static uint8_t write_file_buffer[BUFFER_SIZE];
static uint32_t buffer_index = 0;
static bool transfer_complete = false;
static bool start_byte_flag = true;

/* FUNCTION PROTOTYPES */
static void write_config_file_handle(const uint8_t *data, uint32_t len);
static void process_received_data(void);

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
    ESP_LOGI("PACKET", "crc: 0x%lX", packet.crc);
    if (true != status)
    {
        return CRC_CHECK_ERROR;
    }

    ESP_LOGI("PACKET", "start_byte: 0x%X", packet.start_byte);
    ESP_LOGI("PACKET", "packet_type: 0x%X", packet.packet_type);
    ESP_LOGI("PACKET", "length: 0x%lX", packet.length);

    switch (packet.packet_type)
    {
    case BLE_READ_CONFIG_REQUEST:
        ESP_LOGI("BLE_READ_CONFIG_REQUEST", "Received: BLE_READ_CONFIG_REQUEST");
        status = write_data(data, (HEADER_SIZE + CRC_LENGTH + packet.length));
        break;
    case BLE_VERSION_REQUEST:
        ESP_LOGI("BLE_VERSION_REQUEST", "Received: BLE_VERSION_REQUEST");
        /* TODO: RIGHT NOW HARDCODED. NEED TO BE UNCOMMENTED WHEN VERSION REQUEST FUNCTIONALITY IMPLEMENTED */
        // status = uart_write_bytes(UART_PORT, data, (10 + packet.length));
        break;
    case BLE_WRITE_CONFIG_REQUEST:
        ESP_LOGI("BLE_WRITE_CONFIG_REQUEST", "Received:%ld", packet.length);
        write_config_file_handle(&data[6], packet.length);
        break;
    default:
        ESP_LOGI("PACKET ERROR", "Received: INVALID REQUEST %ld", (int32_t)packet.packet_type);
        status = INVALID_PACKET_ERROR;
        break;
    }
    if (status > 0)
    {
        status = BLE_ACK;
    }
    return status;
}

/**
 * @brief Handles writing the configuration file data into storage.
 *
 * This function processes the incoming configuration file data 
 * and writes it into the internal configuration file buffer. 
 * It may perform boundary checks to prevent buffer overflows 
 * and prepare the data for later use or transmission.
 *
 * @param data   Pointer to the received configuration file data.
 * @param len    Length of the configuration file data in bytes.
 *
 * @return None.
 */
static void write_config_file_handle(const uint8_t *data, uint32_t len) {
    if (start_byte_flag == true) {
        start_byte_flag = false;
        write_file_buffer[0] = START_BYTE;
        write_file_buffer[1] = BLE_WRITE_CONFIG_REQUEST;
        buffer_index += HEADER_SIZE;
    }

    memcpy(&write_file_buffer[buffer_index], data, len);
    buffer_index += len;

    if (len < CHUNK_SIZE) {
        transfer_complete = true;
    }
    
    process_received_data();
}

/**
 * @brief Processes the data received and stored in the internal buffer.
 *
 * This function performs parsing, validation (such as CRC checks), and 
 * handles the received data appropriately. Depending on the data type, 
 * it may update system configurations, trigger further actions, or 
 * prepare a response.
 *
 * @note This function assumes that the data is already stored in a 
 *       global or static buffer and its length is correctly maintained.
 *
 * @return None.
 */
static void process_received_data(void) {
    if (!transfer_complete)
        return;

    uint32_t calc_crc = 0;
    calc_crc = crc32(write_file_buffer, buffer_index);
    ESP_LOGI("CRC: ", "%ld", calc_crc);
    u32_to_byte_array_little_endian(&write_file_buffer[2], buffer_index);
    u32_to_byte_array_little_endian(&write_file_buffer[buffer_index], calc_crc);
    write_data((const char *)write_file_buffer, buffer_index + CRC_LENGTH);
    printf("Sent %ld bytes to UART\n", buffer_index + CRC_LENGTH);

    // Reset buffer for next transfer
    buffer_index = 0;
    transfer_complete = false;
    start_byte_flag = true;
}
