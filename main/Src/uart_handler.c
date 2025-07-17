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
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_handler.h"
#include "crc.h"
#include "utility.h"
#include "ble_response_handler.h"

static const char *TAG = "UART";
#define UART_PORT UART_NUM_1

// In your header file or at top of C file

#define RD_BUF_SIZE (1024)

#define TXD_PIN ((gpio_num_t)GPIO_NUM_17)
#define RXD_PIN ((gpio_num_t)GPIO_NUM_18)
#define RTS_PIN ((gpio_num_t)GPIO_NUM_19)
#define CTS_PIN ((gpio_num_t)GPIO_NUM_20)

#define BAUD_RATE (1000000)

#define FILE_SIZE (5488)

static uart_write_state_t uart_state = UART_READ_START;
int32_t uart_read_index = 0;

/* FUNCTION PROTOTYPE */
static void uart_read_state_machine(uint8_t* data, uint32_t len);
static void initialize_uart_vars(void);


extern void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);

char dtmp[RD_BUF_SIZE];
static QueueHandle_t uart1_queue;

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
esp_err_t uart_initialization(void)
{
    esp_err_t ret;
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // Install UART driver, and get the queue.
    ret = uart_driver_install(UART_PORT, RD_BUF_SIZE * 15, RD_BUF_SIZE * 2, 80, &uart1_queue, 0);
    if (ret != ESP_OK) return ret;
    // CONFIGURE UART 
    ret = uart_param_config(UART_PORT, &uart_config);
    if (ret != ESP_OK) return ret;

    // Set UART pins (using UART0 default pins ie no changes.)
    ret = uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) return ret;
    return ret;
}

/**
 * @brief Resets the variables used in UART.
 *
 * @param None
 *
 * @return None.
 */
static void initialize_uart_vars(void) {
    uart_state = UART_READ_START;
    uart_read_index = 0;
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
static void uart_read_state_machine(uint8_t* data, uint32_t len) {
    error_code_t status = SUCCESS;
    
    ESP_LOGI("UART_READ: ", "%d", uart_state);
    switch (uart_state) {
    case UART_READ_START:
        ESP_LOGI("UART_READ: ", "UART_READ_START");
        if (data[0] != START_BYTE) 
        {
            ESP_LOGI("UART_READ: ", "ERROR");
            status = START_BYTE_ERROR;
            uart_write_bytes(UART_PORT, &status, 1);
            uart_state = UART_READ_END;
            break;
        }
        switch (data[1])
        {
        case BLE_READ_CONFIG_RESPONSE:
        case BLE_VERSION_RESPONSE:
            set_data_read_length(byte_array_to_u32_little_endian(&data[2]));
            reset_buffer_variables();
            store_config_file(&data[6], (len-6));
            uart_state = UART_READ_DATA;
            /* TODO: CAN BE IMPLEMENTED ONCE IMPLEMENTATION DONE IN NEXUS*/
            break;
        default:
            status = INVALID_PACKET_ERROR;
            uart_write_bytes(UART_PORT, &status, 1);
            uart_state = UART_READ_START;
            break;
        }          
        break;
    case UART_READ_DATA:
        store_config_file(data, len);
        break;
    default:
        ESP_LOGI("UART_READ: : ", "UART_READ_END");
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
    ESP_LOGI("UART TASK: ", "Task Created");
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            ESP_LOGI(TAG, "uart[%d] event", event.type);
            switch (event.type)
            {
            /* UART DATA EVENT */
            case UART_DATA:
                uart_read_bytes(UART_PORT, dtmp, event.size, (TickType_t)portMAX_DELAY);
                uart_read_state_machine((uint8_t*)dtmp, event.size);
                uart_read_index += event.size;
                uart_get_buffered_data_len(UART_PORT, &buffered_size);
                ESP_LOGI("UART READ", "[LEN: ]: %ld", uart_read_index);
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
 * @brief Writes data into the internal buffer.
 *
 * This function copies the provided data into the internal 
 * data buffer starting from the current write offset. It handles 
 * boundary checks to ensure that the buffer does not overflow.
 *
 * @param src    Pointer to the source data to be written.
 * @param size   Number of bytes to write.
 *
 * @return  0 on success,
 *         -1 if input is invalid,
 *         -2 if write exceeds buffer capacity.
 */
int32_t write_data(const void *src, uint32_t size)
{
    initialize_uart_vars();
    return uart_write_bytes(UART_PORT, src, size);
}

