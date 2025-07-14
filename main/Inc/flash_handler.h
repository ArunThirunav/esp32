#ifndef FLASH_HANDLER_H
#define FLASH_HANDLER_H

#include "esp_littlefs.h"
#include <stdint.h>

int32_t flash_initalization(void);
uint32_t get_total_flash_size(uint32_t* total, uint32_t* used);
int32_t write_data_to_flash(const uint8_t* data, uint32_t len);
int32_t read_data_from_flash(uint8_t* data, uint32_t len);
int32_t format_flash(void);

#endif /* END OF FLASH_HANDLER_H */