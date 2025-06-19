#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_handler.h"

static const char *TAG = "UART";
#define UART_PORT UART_NUM_1

// In your header file or at top of C file
#define BUF_SIZE (1024)
#define BUFFER_SIZE (128 * BUF_SIZE)  // 256KB

// Global declaration (uses DRAM)
static uint8_t global_buffer[BUFFER_SIZE];

#define RD_BUF_SIZE (BUF_SIZE)

#define TXD_PIN ((gpio_num_t)GPIO_NUM_17)
#define RXD_PIN ((gpio_num_t)GPIO_NUM_18)
#define RTS_PIN ((gpio_num_t)GPIO_NUM_19)
#define CTS_PIN ((gpio_num_t)GPIO_NUM_20)

char dtmp[BUF_SIZE];
static QueueHandle_t uart1_queue;

int parse_data(const uint8_t* data){
    esp_err_t status = ESP_OK;
    switch (data[1]) {
    case BLE_CONFIG_REQUEST:
        ESP_LOGI(TAG, "Received: BLE_CONFIG_REQUEST");
        uart_write_bytes(UART_PORT, (const char *)data, 0x04);
        break;    
    default:
        ESP_LOGI(TAG, "Received: INVALID REQUEST %d", data[1]);
        status = ESP_FAIL;
        break;
    }
    return status;
}

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
	size_t buffered_size;
    bzero(dtmp, RD_BUF_SIZE);
	int index = 0;
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            ESP_LOGI(TAG, "uart[%d] event", UART_PORT);
            switch (event.type) {
            /* UART DATA EVENT */
            case UART_DATA:
				ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
				uart_read_bytes(UART_PORT, dtmp, event.size, (TickType_t)portMAX_DELAY);
				memcpy(global_buffer+index, dtmp, event.size);
				index += event.size;
				uart_get_buffered_data_len(UART_PORT, &buffered_size);
				ESP_LOGI(TAG, "[DATA INDEX]: %d and %d", index, buffered_size); 
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

void uart_initialization(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(UART_PORT, BUF_SIZE * 15, BUF_SIZE * 2, 80, &uart1_queue, 0);
    uart_param_config(UART_PORT, &uart_config);

    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

}