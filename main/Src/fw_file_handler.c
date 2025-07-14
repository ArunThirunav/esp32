#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "fw_file_handler.h"
#include "flash_handler.h"
#include "utility.h"
#include "esp_log.h"
#include <string.h>

#define TAG "FLASH_WRITER"
#define FLASH_BLOCK_SIZE 8192
#define ITEM_COUNT 40
#define ITEM_SIZE 256
extern uint8_t write_file_buffer[BUFFER_SIZE];

static uint8_t ring_buffer[FLASH_BLOCK_SIZE];
static uint32_t write_index = 0;
static uint32_t counter = 0;
static uint32_t buffer_offset = 0;
static bool last_packet_flag = false;
static int active_half = 0; // 0 = first half, 1 = second half

TaskHandle_t fw_file_write_handle;
FILE *flash_fp = NULL;

void ble_data_write_task(void *param)
{
    uint8_t chunk[256];
    size_t chunk_len;

    ESP_LOGI("FILE_HANDLE: ", "QUEUE CREATED");



    while (1) {
        // Wait for any of the 3 notifications
        uint32_t notifiedIndex;
        BaseType_t notified = xTaskNotifyWaitIndexed(0, 0xFFFFFFFF, 0xFFFFFFFF, &notifiedIndex, portMAX_DELAY);

        if (notified == pdTRUE) {
            switch (notifiedIndex) {
                case HALF_0_READY:
                    printf("Writing buffer[0] to flash\n");
                    write_data_to_flash(&write_file_buffer[0], HALF_SIZE);
                    break;

                case HALF_1_READY:
                    printf("Writing buffer[1] to flash\n");
                    write_data_to_flash(&write_file_buffer[HALF_SIZE], HALF_SIZE);
                    break;

                case FINAL_FLUSH: {
                    // Determine the last written half and partial length
                    int flushing_half = active_half;
                    size_t base_offset = flushing_half * HALF_SIZE;
                    printf("Flushing final %ld bytes from half %d\n", buffer_offset, flushing_half);

                    if (buffer_offset > 0) {
                        write_data_to_flash(&write_file_buffer[base_offset], buffer_offset);
                        buffer_offset = 0;  // reset
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void set_last_packet(uint8_t half, uint32_t offset) {
    active_half = half;
    buffer_offset = offset;
    ESP_LOGI("FW_FILE: ", "Last Packet: %ld", offset);
}

int32_t erase_flash(void){
    write_index = 0;
    counter = 0;
    return format_flash();
}
