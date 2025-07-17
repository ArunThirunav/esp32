#ifndef FW_FILE_HANDLE_H
#define FW_FILE_HANDLE_H

#include <stdint.h>
#include <stdbool.h>

/* DEFINES */
#define BUF_SIZE (1024)
#define FW_FILE_CHUNK (256)
#define BUFFER_SIZE ((FW_FILE_CHUNK * BUF_SIZE) + HEADER_SIZE + CRC_LENGTH) // 256KB
#define CHUNK_SIZE (500)
#define HALF_SIZE (1024 * 128)

typedef enum {
    HALF_0_READY = 0,
    HALF_1_READY = 1,
    FINAL_FLUSH  = 2  // New flag for last partial write
} BufferNotifyIndex;

void ble_data_write_task(void *param);
void set_last_packet(uint8_t half, uint32_t offset);
int32_t erase_flash(void);
int send_flash_task(void);

#endif /* END OF FW_FILE_HANDLE_H */