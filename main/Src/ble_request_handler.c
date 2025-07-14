/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : ble_request_handler.c
 * @brief          : Handle the configuration file
 ******************************************************************************
 */
/* USER CODE END Header */

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ble_request_handler.h"
#include "fw_file_handler.h"
#include "crc.h"
#include "utility.h"


/* VARIABLES */
__attribute__((section(".dram2.bss"))) uint8_t write_file_buffer[BUFFER_SIZE];
static uint32_t buffer_index = 0;
static bool transfer_complete = false;
static bool start_byte_flag = true;
static uint32_t count = 0;
static int active_half = 0; // 0 = first half, 1 = second half
static size_t buffer_offset = 0; // Offset within current half

extern TaskHandle_t fw_file_write_handle;

/* FUNCTION PROTOTYPES */
static void write_config_file_handle(const uint8_t *data, uint32_t len);
static void process_received_data(void);
extern uint32_t get_total_flash_size(uint32_t* total, uint32_t* used);
static error_code_t ble_fw_file_handler(uart_data_pack_t* packet);
uint32_t total, used;

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

    if (packet.length != 0 && packet.length <= PAYLOAD_LEN) {
        memcpy(packet.payload, &data[6], packet.length);
    }

    status = validate_crc(data, (6 + packet.length), packet.crc);
    if (true != status)
    {
        ESP_LOGI("PACKET ERROR", "crc: 0x%lX", packet.crc);
        return CRC_CHECK_ERROR;
    }

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
    case BLE_FW_UPLOAD_START:
        /* ERASE THE FLASH REGION */
        ESP_LOGI("BLE_FW_UPLOAD_START", "Erasing");
        buffer_index = 0;
        status = erase_flash();
        break;
    case BLE_FW_UPLOAD_DATA:
        /* WRITE THE FW DATA TO FLASH */
        ESP_LOGI("BLE_FW_UPLOAD_DATA", "%ld", (++count * packet.length));
        status = ble_fw_file_handler(&packet);      
        break;
    case BLE_FW_UPLOAD_END:
        /* SEND NOTIFICATION TO MOBILE AND NEXUS */
        ESP_LOGI("BLE_FW_UPLOAD_END", "Notify");
        get_total_flash_size(&total, &used);
        ESP_LOGI("FLASH_USED: ", "Total: %ld, Used: %ld", total, used);
        buffer_index = 0;
        count = 0;
        break;
    default:
        ESP_LOGI("PACKET ERROR", "Received: INVALID REQUEST %ld", (int32_t)packet.packet_type);
        status = INVALID_PACKET_ERROR;
        break;
    }
    if (status > 0){
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

static error_code_t ble_fw_file_handler(uart_data_pack_t* packet){
    error_code_t rc = ESP_OK;
    return rc;
    bool last_packet = false;
    size_t base_offset = active_half * HALF_SIZE;

    last_packet = (packet->length < CHUNK_SIZE) ? true : false;
    if (last_packet == true) {
        set_last_packet(active_half, buffer_offset);
    }
    
    if (buffer_offset + packet->length > HALF_SIZE) {
         // Only part of the packet fits in current half
        uint32_t remaining_space = HALF_SIZE - buffer_offset;

        if (remaining_space > 0) {
            memcpy(&write_file_buffer[base_offset + buffer_offset], packet->payload, remaining_space);
        }

        // Notify current half full
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveIndexedFromISR(fw_file_write_handle, active_half, &xHigherPriorityTaskWoken);

        // Switch to next half
        active_half = 1 - active_half;
        buffer_offset = 0;

        // Write the leftover part to the new half
        size_t leftover_len = packet->length - remaining_space;
        memcpy(&write_file_buffer[active_half * HALF_SIZE + buffer_offset], packet->payload + remaining_space, leftover_len);
        buffer_offset += leftover_len;

        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }

    else {
        memcpy(&write_file_buffer[base_offset + buffer_offset], packet->payload, packet->length);
        buffer_offset += packet->length;
        if (buffer_offset >= HALF_SIZE) {
            // Notify flash write task about which half is ready
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveIndexedFromISR(fw_file_write_handle, active_half, &xHigherPriorityTaskWoken);
            
            // Switch to other half
            active_half = 1 - active_half;
            buffer_offset = 0;
            
            if (xHigherPriorityTaskWoken) {
                portYIELD_FROM_ISR();
            }
        }
    }
    
    return rc;
    
}   
