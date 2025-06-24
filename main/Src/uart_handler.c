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
#define NEXUS_SIMULATOR 0

// In your header file or at top of C file
#define BUF_SIZE (1024)
#define FW_FILE_CHUNK (128)
#define BUFFER_SIZE (FW_FILE_CHUNK * BUF_SIZE)  // 256KB

// Global declaration (uses DRAM)
static uint8_t global_buffer[BUFFER_SIZE];

#define RD_BUF_SIZE (BUF_SIZE)

#define TXD_PIN ((gpio_num_t)GPIO_NUM_17)
#define RXD_PIN ((gpio_num_t)GPIO_NUM_18)
#define RTS_PIN ((gpio_num_t)GPIO_NUM_19)
#define CTS_PIN ((gpio_num_t)GPIO_NUM_20)

char dtmp[BUF_SIZE];
static QueueHandle_t uart1_queue;
static uart_write_state_t uart_state = UART_WRITE_START;
static uint32_t data_read_length = 0;

static void uart_write_state_machine(uint8_t* data, uint32_t len);

void junk_fill(void) {
	int counter = 0;
	memset(global_buffer, '\0', sizeof(global_buffer));
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 500; j++){
            global_buffer[counter++] = 65+i;
        }
    }
	// for (int i = 0; i < 5000; i++) {
	// 	printf("%d", global_buffer[i]);
	// }	
}

/**
 * @brief
 * @param
 * @return
 * */
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

    #if NEXUS_SIMULATOR
    junk_fill();
    #endif /* END OF NEXUS_SIMULATOR */
}

/**
 * @brief
 * @param
 * @return
 * */
int uart_data_handler(const uint8_t* data){
    error_code_t status = ESP_OK;
    uart_data_pack_t packet;

    packet.start_byte = data[0];
	packet.packet_type = data[1];
	packet.length = byte_array_to_u32_little_endian(&data[2]);
    if(packet.length != 0) {
        memcpy(packet.payload, &data[3], packet.length);
    }

	/* 6 is the 1(start byte)+1(packet_type)+4(length) */
	packet.crc = byte_array_to_u32_little_endian(&data[6 + packet.length]); 
    if (packet.length != 0 && packet.length <= PAYLOAD_LEN) {
        memcpy(packet.payload, &data[3], packet.length);
    }
    
    status = validate_crc(data, (6 + packet.length), packet.crc);
    if (true != status) {
        return CRC_CHECK_ERROR;
    }

    ESP_LOGI("PACKET", "start_byte: 0x%X", packet.start_byte);
	ESP_LOGI("PACKET", "packet_type: 0x%X", packet.packet_type);
	ESP_LOGI("PACKET", "length: 0x%lX", packet.length);
	ESP_LOGI("PACKET", "crc: 0x%lX", packet.crc);

    switch (packet.packet_type) {
    case BLE_CONFIG_REQUEST:
        ESP_LOGI(TAG, "Received: BLE_CONFIG_REQUEST");
        status = uart_write_bytes(UART_PORT, (char *)&packet, (10 + packet.length));
        break;
    case BLE_VERSION_REQUEST:
        ESP_LOGI(TAG, "Received: BLE_VERSION_REQUEST");
        status = uart_write_bytes(UART_PORT, (char *)&packet, (10 + packet.length));
        break;
    default:
        ESP_LOGI(TAG, "Received: INVALID REQUEST %d", packet.packet_type);
        status = INVALID_PACKET_ERROR;
        break;
    }
    return status;
}

/**
 * @brief
 * @param
 * @return
 * */
#define FILE_SIZE (5488)
#define CHUNK_SIZE (500)
int current_offset = 0;
int sent = 0;
uint8_t* uart_read_data(uint32_t* length){
	int remaining = FILE_SIZE - current_offset;
	int prev_offset = 0;
    int len = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

	ESP_LOGI("READ: ", "Sent: %d", sent += len);

	prev_offset = current_offset;
	*length = len;
	current_offset += len;

	if (current_offset >= FILE_SIZE) {
        current_offset = 0;
    }

    return &global_buffer[prev_offset];
}

void set_data_read_length(uint32_t length) {
    ESP_LOGI("DATA_LEN: ", "%ld", length);
    data_read_length = length;
}

uint32_t get_data_read_length(void) {
    ESP_LOGI("DATA_LEN: ", "%ld", data_read_length);
    return data_read_length;
}

static void uart_write_state_machine(uint8_t* data, uint32_t len) {
    error_code_t status = SUCCESS;
    static uint32_t counter = 0;
    ESP_LOGI("TAG", "Came");
    switch (uart_state) {
    case UART_WRITE_START:
        if (data[0] != START_BYTE) {
            status = START_BYTE_ERROR;
            uart_state = UART_WRITE_END;
            break;
        }
        else {
            set_data_read_length(byte_array_to_u32_little_endian(&data[2]));
            uart_state = UART_WRITE_DATA;
        }
        memcpy(global_buffer+counter, &data[6], (len - 6));
        counter += len;
        break;
    case UART_WRITE_DATA:
        if (get_data_read_length() < counter) {
            uart_state = UART_WRITE_END;
        }
        else{
            memcpy(global_buffer+counter, data, len);
            counter += len;
        }
        break;
    case UART_WRITE_END:
        uart_state = UART_WRITE_START;
        break;
    default:
        break;
    }
}

/**
 * @brief
 * @param
 * @return
 * */
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
                uart_write_state_machine((uint8_t*)dtmp, event.size);
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

