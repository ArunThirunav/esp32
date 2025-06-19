#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "uart_events";

#define UART_SEL UART_NUM_1

// In your header file or at top of C file
#define BUFFER_SIZE (256 * 1024)  // 256KB

// Global declaration (uses DRAM)
static uint8_t global_buffer[BUFFER_SIZE];

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define TXD_PIN ((gpio_num_t)GPIO_NUM_17)
#define RXD_PIN ((gpio_num_t)GPIO_NUM_18)

char dtmp[BUF_SIZE];
// char rx_buff[500];

static QueueHandle_t uart1_queue;

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
	size_t buffered_size;
    bzero(dtmp, RD_BUF_SIZE);
	int index = 0;
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            ESP_LOGI(TAG, "uart[%d] event", UART_SEL);
            switch (event.type) {
            case UART_DATA:
				ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
				uart_read_bytes(UART_SEL, dtmp, event.size, (TickType_t)portMAX_DELAY);
				memcpy(global_buffer+index, dtmp, event.size);
				// printf("String: %s", &global_buffer[index]);
				// printf("dtemp: %s", dtmp);
				index += event.size;
				uart_get_buffered_data_len(UART_SEL, &buffered_size);
				ESP_LOGI(TAG, "[DATA INDEX]: %d and %d", index, buffered_size); 
				
				break;
			case UART_PATTERN_DET:
            	printf("Data Received\n");
				
				break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_SEL);
                xQueueReset(uart1_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider increasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_SEL);
                xQueueReset(uart1_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 460800,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(UART_SEL, BUF_SIZE * 15, BUF_SIZE * 2, 80, &uart1_queue, 0);
    uart_param_config(UART_SEL, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_MAX);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_SEL, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 3*1024, NULL, 3, NULL);
}

/* 
115200, 6 
230400, 10 error received only 7320
460800, 15
921600

*/