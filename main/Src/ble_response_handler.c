#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "host/ble_hs.h"
#include "esp_log.h"
#include "ble_response_handler.h"
#include "crc.h"
#include "utility.h"

/* DEFINES */
#define BUF_SIZE (1024)
#define FW_FILE_CHUNK (20)
#define BUFFER_SIZE (FW_FILE_CHUNK * BUF_SIZE) // 256KB
#define CHUNK_SIZE (500)

/* VARIABLES */
static uint32_t data_read_length = 0;
static uint8_t read_file_buffer[BUFFER_SIZE];
static uint32_t uart_write_index = 0;
extern uint16_t notify_config_handle;
int32_t current_offset = 0;
uint32_t sent = 0;
static uint16_t connection_hndl = 0;

/* FUNCTION PROTOTYPES */
void send_config_file(void);

/**
 * @brief Resets all buffer-related variables to initial state.
 *
 * This function clears and resets buffer management variables 
 * such as buffer pointers, counters, data length trackers, and 
 * state machine statuses. It is typically used at the start or 
 * end of a communication session or after an error recovery.
 *
 * @note This function only resets control variables; if buffer 
 * memory needs to be cleared, it should be handled separately.
 *
 * @return None.
 */
void reset_buffer_variables(void)
{
    uart_write_index = 0;
    sent = 0;
    current_offset = 0;
}

/**
 * @brief Sets the expected data read length.
 *
 * This function configures the length of data to be read in the 
 * next read operation. It is typically used in protocols where 
 * the amount of incoming data varies and needs to be set dynamically 
 * before initiating a read.
 *
 * @param length   The number of bytes to read in the next read 
 *                 or data reception operation.
 *
 * @return None.
 */
void set_data_read_length(uint32_t length) {
    ESP_LOGI("SET DATA_LEN: ", "%ld", length);
    data_read_length = length;
}

/**
 * @brief Retrieves the current expected data read length.
 *
 * This function returns the number of bytes previously configured 
 * for the next data read operation using set_data_read_length().
 * @param None
 * @return  The expected data read length in bytes.
 */
uint32_t get_data_read_length(void) {
    return data_read_length;
}

/**
 * @brief Stores the configuration file data into internal buffer.
 *
 * This function saves the provided configuration data into an 
 * internal buffer. It is typically 
 * called when receiving a configuration file over UART from Nexus
 *
 * @param data    Pointer to the data buffer containing the configuration file.
 * @param length  Length of the configuration data in bytes.
 *
 * @return None.
 */
void store_config_file(uint8_t* data, uint32_t length) 
{
    memcpy(read_file_buffer + uart_write_index, data, length);        
    ESP_LOGI("UART_WRITE_INDEX", "%ld", uart_write_index);
    uart_write_index += length;
    ESP_LOGI("BYTES RECEIVED: ", "%ld", uart_write_index);

    if (uart_write_index >= get_data_read_length()) {
        ESP_LOGI("READ COMPLETE", "SEND NOTIFICATION");
        send_config_file();
    }
}

/**
 * @brief Sets the current BLE connection handle.
 *
 * This function saves the connection handle associated with 
 * an active BLE connection. It is typically called during 
 * connection events to store the handle for future communication 
 * such as notifications, indications, or connection management.
 *
 * @param hndl   The BLE connection handle.
 *
 * @return None.
 */
void set_connection_handle(uint16_t hndl) {
    connection_hndl = hndl;
}

/**
 * @brief Retrieves the current BLE connection handle.
 *
 * This function returns the connection handle previously stored 
 * using set_connection_handle(). The connection handle is used 
 * for sending BLE notifications, indications, or managing the 
 * active BLE connection.
 *
 * @return The currently stored BLE connection handle.
 *         Returns 0xFFFF if no active connection exists.
 */
uint16_t get_connection_handle(void) {
    return connection_hndl;
}

/**
 * @brief Provides access to the global data buffer.
 *
 * This function returns a pointer to the global data buffer and 
 * optionally provides the current valid data length stored in the buffer.
 *
 * @param[out] length   Pointer to a variable where the buffer length 
 *                      will be stored. Pass NULL if length is not needed.
 *
 * @return Pointer to the global data buffer.
 */
uint8_t *global_buffer_read(uint32_t *length)
{
    int32_t remaining = get_data_read_length() - current_offset;
    int32_t prev_offset = 0;
    int32_t len = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

    ESP_LOGI("READ: ", "Sent: %ld", sent += *length);

    prev_offset = current_offset;
    *length = len;
    current_offset += len;

    if (current_offset >= get_data_read_length())    
    {
        current_offset = 0;
    }

    return &read_file_buffer[prev_offset];
}

/**
 * @brief Sends the stored configuration file to the connected client.
 *
 * This function transmits the configuration file data from the 
 * internal buffer to the client over BLE (via notifications).
 * It handles chunked transmission if the file size exceeds 
 * the MTU or buffer limits.
 *
 * @note Requires an active connection handle. The function typically 
 * checks connection validity before attempting transmission.
 *
 * @return None.
 */
void send_config_file(void)
{
    uint8_t temp[517];
    uint32_t crc_calc;
    uint32_t length;
    uint16_t conn_hndl = 0;
    struct os_mbuf *om;
    conn_hndl = get_connection_handle();
    if (conn_hndl == 0)
    {
        return;
    }
    
    while (1){
        memset(temp, '\0', sizeof(temp));

        temp[0] = START_BYTE;
        temp[1] = BLE_READ_CONFIG_RESPONSE;
        
        const uint8_t *resp = global_buffer_read(&length);
        u32_to_byte_array_little_endian(&temp[2], length);
        memcpy(&temp[HEADER_SIZE], resp, (length));
        crc_calc = crc32(temp, (length) + HEADER_SIZE);
        u32_to_byte_array_little_endian(&temp[HEADER_SIZE + (length)], crc_calc);
        length += 10;

        ESP_LOGI("NOTIFY DATA LENGTH: ", "%ld", length);
        om = ble_hs_mbuf_from_flat(&temp, length);
        ble_gattc_notify_custom(conn_hndl, 
                                     notify_config_handle, 
                                     om);
        if(length < 510){
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
