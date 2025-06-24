#include "config_file_handle.h"
#include "crc.h"
#include "utils.h"
#include <string.h>
uint8_t temp[517];

static uint32_t config_file_size = 0;

uint8_t* get_config_file(uint32_t* length) {

    uint32_t crc_calc;
    temp[0] = START_BYTE;
	temp[1] = BLE_CONFIG_RESPONSE;
	const uint8_t *resp = uart_read_data(length);
    u32_to_byte_array_little_endian(&temp[2], *length);   
    memcpy(&temp[6], resp, (*length));
	crc_calc = crc32(temp, (*length) + 6);
    u32_to_byte_array_little_endian(&temp[6+(*length)], crc_calc); 
    *length += 10;
    return &temp[0];
}

uint32_t get_config_file_size(void) {
    return config_file_size;
}
void set_config_file_size(uint32_t len) {
    config_file_size = len;
}
