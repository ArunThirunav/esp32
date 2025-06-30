/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : uart_handler.c
 * @brief          : Handle the UART functionality
 ******************************************************************************
 */
/* USER CODE END Header */

#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_handler.h"
#include "crc.h"
#include "utils.h"

static const char *TAG = "UART";
#define UART_PORT UART_NUM_1

// In your header file or at top of C file
#define BUF_SIZE (1024)
#define FW_FILE_CHUNK (128)
#define BUFFER_SIZE (FW_FILE_CHUNK * BUF_SIZE) // 256KB

#define RD_BUF_SIZE (BUF_SIZE)

#define TXD_PIN ((gpio_num_t)GPIO_NUM_17)
#define RXD_PIN ((gpio_num_t)GPIO_NUM_18)
#define RTS_PIN ((gpio_num_t)GPIO_NUM_19)
#define CTS_PIN ((gpio_num_t)GPIO_NUM_20)

#define BAUD_RATE (115200)

#define FILE_SIZE (5488)
#define CHUNK_SIZE (500)

// #define APP

// Global declaration (uses DRAM)
static uart_write_state_t uart_state = UART_WRITE_START;
static uint8_t global_buffer[BUFFER_SIZE];
extern bool response_flag;
extern bool config_request_flag;
static uint32_t data_read_length = 0;
static uint32_t uart_write_index = 0;
int uart_read_index = 0;
int current_offset = 0;
int sent = 0;

static void uart_write_state_machine(uint8_t* data, uint32_t len);
static void reset_uart_configuration(void);
static void set_data_read_length(uint32_t length);
static uint32_t get_data_read_length(void);

char dtmp[BUF_SIZE];
static QueueHandle_t uart1_queue;

/**
 * @brief Fills a predefined buffer with junk (dummy) data.
 *
 * This function populates a buffer with dummy or test data, often 
 * used for testing memory, simulating data transmissions, or 
 * debugging. The fill pattern can be random, incremental, or a 
 * constant pattern based on the implementation.
 *
 * @note The buffer and its size should be defined within the function 
 * or globally. This function does not take parameters or return values.
 *
 * @return None.
 */
void junk_fill(void)
{
    int counter = 0;
    memset(global_buffer, '\0', sizeof(global_buffer));
    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 500; j++)
        {
            global_buffer[counter++] = 65 + i;
        }
    }
    // for (int i = 0; i < 5000; i++) {
    // 	printf("%d", global_buffer[i]);
    // }
}

/**
 * @brief Initializes the UART peripheral.
 *
 * This function configures and initializes the UART interface with 
 * predefined settings such as baud rate, data bits, stop bits, parity, 
 * and flow control. It prepares the UART for transmission and reception 
 * of data.
 *
 * @note This function must be called before any UART send or receive 
 * operations.
 *
 * @return None.
 */
void uart_initialization(void)
{
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // Install UART driver, and get the queue.
    uart_driver_install(UART_PORT, BUF_SIZE * 15, BUF_SIZE * 2, 80, &uart1_queue, 0);
    uart_param_config(UART_PORT, &uart_config);

    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    #ifdef APP
    junk_fill();
    #endif
}

/**
 * @brief Resets the variables used in UART.
 *
 * @param None
 *
 * @return None.
 */
static void reset_uart_configuration(void) {
    uart_write_index = 0;
    uart_state = UART_WRITE_START;
    uart_read_index = 0;
    current_offset = 0;
    sent = 0;
    memset(global_buffer, '\0', sizeof(global_buffer));
}

/**
 * @brief
 * @param
 * @return
 * */

uint8_t *uart_read_data(uint32_t *length)
{
    #ifdef APP
    int remaining = FILE_SIZE - current_offset;
    #else
    int remaining = get_data_read_length() - current_offset;
    #endif 
    int prev_offset = 0;
    int len = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

    ESP_LOGI("READ: ", "Sent: %d", sent += len);

    prev_offset = current_offset;
    *length = len;
    current_offset += len;

    #ifdef APP
    if (current_offset >= FILE_SIZE)
    #else
    if (current_offset >= get_data_read_length())
    #endif 
    
    {
        current_offset = 0;
    }

    return &global_buffer[prev_offset];
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
static void set_data_read_length(uint32_t length) {
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
static uint32_t get_data_read_length(void) {
    ESP_LOGI("SET DATA_LEN: ", "%ld", data_read_length);
    return data_read_length;
}

/**
 * @brief UART write state machine to transmit data over UART.
 *
 * This function manages the UART transmission process using a state 
 * machine approach. It handles chunked sending of large data buffers, 
 * manages transmission states, and ensures proper sequencing of UART 
 * data transfers.
 *
 * @param data   Pointer to the data buffer to transmit.
 * @param len    Length of the data in bytes.
 *
 * @return None.
 */
static void uart_write_state_machine(uint8_t* data, uint32_t len) {
    error_code_t status = SUCCESS;
    
    ESP_LOGI("TAG", "%d", uart_state);
    switch (uart_state) {
    case UART_WRITE_START:
        ESP_LOGI("TAG", "UART_WRITE_START");
        for (int i = 0; i < 10; i++)
        {
            ESP_LOGI("RECV: ", "%X", data[i]);
        }
        if (data[0] != START_BYTE) {
            ESP_LOGI("TAG", "ERROR");
            status = START_BYTE_ERROR;
            uart_write_bytes(UART_PORT, &status, 1);
            uart_state = UART_WRITE_END;
            break;
        }
        else {
            uint32_t length = byte_array_to_u32_little_endian(&data[2]);
            ESP_LOGI("TAG", "%ld", length);
            set_data_read_length(length);
            uart_state = UART_WRITE_DATA;
        }
        memcpy(global_buffer+uart_write_index, &data[6], (len - 6));
        uart_write_index += len;
        break;
    case UART_WRITE_DATA:
        if (get_data_read_length() <= uart_write_index) {
            ESP_LOGI("TAG", "COMPLETED");
            uart_write_index = 0;
            uart_state = UART_WRITE_END;
        }
        else{
            memcpy(global_buffer+uart_write_index, data, len);
            uart_write_index += len;
            ESP_LOGI("BYTES RECEIVED: ", "%ld", uart_write_index);
        }
        break;
    case UART_WRITE_END:
        ESP_LOGI("TAG", "UART_WRITE_END");
        uart_state = UART_WRITE_START;
        break;
    default:
        break;
    }
}

/**
 * @brief UART event handling task.
 *
 * This FreeRTOS task waits for UART events from the UART driver event queue, 
 * such as data reception, buffer overflows, framing errors, or pattern detection. 
 * It processes incoming UART data and handles error conditions as needed.
 *
 * @param pvParameters   Pointer to task parameters (typically NULL or 
 *                       a UART configuration/context structure).
 *
 * @return None. This task runs indefinitely.
 */
void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    bzero(dtmp, RD_BUF_SIZE);
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            ESP_LOGI(TAG, "uart[%d] event", UART_PORT);
            switch (event.type)
            {
            /* UART DATA EVENT */
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                uart_read_bytes(UART_PORT, dtmp, event.size, (TickType_t)portMAX_DELAY);
                uart_write_state_machine((uint8_t*)dtmp, event.size);
                // memcpy(global_buffer + uart_read_index, dtmp, event.size);
                uart_read_index += event.size;
                uart_get_buffered_data_len(UART_PORT, &buffered_size);
                ESP_LOGI(TAG, "[DATA INDEX]: %d and %d", uart_read_index, buffered_size);
                break;
            /* UART DATA BREAK EVENT */
            case UART_DATA_BREAK:
                ESP_LOGI(TAG, "UART DATA BREAK");
                break;
            case UART_PATTERN_DET:
                printf("Data Received\n");
                break;
            /* UART FIFO OVERFLOW */
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                uart_flush_input(UART_PORT);
                xQueueReset(uart1_queue);
                break;
            /* UART RING BUFFER OVERFLOW THE RX BUFFER DURING UART DRIVER INSTALL */
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                uart_flush_input(UART_PORT);
                xQueueReset(uart1_queue);
                break;
            /* UART BREAK EVENT */
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            /* UART COMMUNICATION ERROR */
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            /* OTHER EVENT APART FROM THE ABOVE EVENT */
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
}

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
int uart_data_handler(const uint8_t *data)
{
    error_code_t status = ESP_OK;
    uart_data_pack_t packet;

    packet.start_byte = data[0];
    packet.packet_type = data[1];
    packet.length = byte_array_to_u32_little_endian(&data[2]);
    if (packet.length != 0)
    {
        memcpy(packet.payload, &data[3], packet.length);
    }

    /* 6 is the 1(start byte)+1(packet_type)+4(length) */
    packet.crc = byte_array_to_u32_big_endian(&data[6 + packet.length]);
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
        config_request_flag = true;
        ESP_LOGI(TAG, "Received: BLE_CONFIG_REQUEST");
        reset_uart_configuration();
        status = uart_write_bytes(UART_PORT, data, (10 + packet.length));
        break;
    case BLE_VERSION_REQUEST:
        ESP_LOGI(TAG, "Received: BLE_VERSION_REQUEST");
        response_flag = true;
        // status = uart_write_bytes(UART_PORT, data, (10 + packet.length));
        break;
    case BLE_CONFIG_RESPONSE:

        break;
    default:
        ESP_LOGI(TAG, "Received: INVALID REQUEST %d", packet.packet_type);
        status = INVALID_PACKET_ERROR;
        break;
    }
    if (status > 0)
    {
        status = BLE_ACK;
    }
    return status;
}
