#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "fw_file_handler.h"
#include "flash_handler.h"
#include "utility.h"
#include "esp_log.h"
#include <string.h>

#define TAG "FLASH_WRITER"
#define FLASH_BLOCK_SIZE (1024*128)
#define ITEM_COUNT 40
#define ITEM_SIZE 256
extern uint8_t write_file_buffer[BUFFER_SIZE];

static uint32_t buffer_offset = 0;
static bool last_packet_flag = false;
static int active_half = 0; // 0 = first half, 1 = second half
static int counter = 0;

TaskHandle_t fw_file_write_handle;
FILE *flash_fp = NULL;

void ble_data_write_task(void *param)
{
    ESP_LOGI("FILE_HANDLE: ", "TASK CREATED");

    while (1) {
        if (ulTaskNotifyTakeIndexed(HALF_0_READY, pdTRUE, pdMS_TO_TICKS(10))) {
            ESP_LOGI("FILE_HANDLE: ", "Writing 1st Half: %d", (++counter) * FLASH_BLOCK_SIZE);
            write_data_to_flash(&write_file_buffer[0], HALF_SIZE);
        }
        if (ulTaskNotifyTakeIndexed(HALF_1_READY, pdTRUE, pdMS_TO_TICKS(10))) {
            ESP_LOGI("FILE_HANDLE: ", "Writing 2nd Half: %d", (++counter) * FLASH_BLOCK_SIZE);
            write_data_to_flash(&write_file_buffer[HALF_SIZE], HALF_SIZE);
        }
        if (ulTaskNotifyTakeIndexed(FINAL_FLUSH, pdTRUE, pdMS_TO_TICKS(10))) {
            ESP_LOGI("FILE_HANDLE: ", "Writing Last Packet: %d", (++counter) * FLASH_BLOCK_SIZE);
            // TO DETERMINE THE LAST WRITTEN HALF AND PARTIAL LENGTH
            int flushing_half = active_half;
            size_t base_offset = flushing_half * HALF_SIZE;
            ESP_LOGI("FILE_HANDLE: ", "Flushing final %ld bytes from half %d", buffer_offset, flushing_half);
            if (buffer_offset > 0) {
                write_data_to_flash(&write_file_buffer[base_offset], buffer_offset);
                buffer_offset = 0;  // reset
            }
        }
    }
}

void set_last_packet(uint8_t half, uint32_t offset) {
    active_half = half;
    buffer_offset = offset;
    ESP_LOGI("FW_FILE: ", "Last Packet: %ld", offset);
}

int send_flash_task(void) {
    get_file_size();
    return 0;
}

int32_t erase_flash(void){
    counter = 0;
    return format_flash();
}
