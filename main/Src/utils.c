#include "utils.h"
#include "esp_log.h"

uint32_t byte_array_to_u32_little_endian(const uint8_t* data) {
    for (int i = 0; i < 4; i++)
    {
        ESP_LOGI("LITTLE: ", "%d", data[i]);
    }
    
    return ((uint32_t)data[3]) << 24 |
            ((uint32_t)data[2] << 16) |
            ((uint32_t)data[1] << 8) |
            ((uint32_t)data[0]); 
}

uint32_t byte_array_to_u32_big_endian(const uint8_t* data) {
    for (int i = 0; i < 4; i++)
    {
        ESP_LOGI("BIG: ", "%d", data[i]);
    }
    return ((uint32_t)data[3])|
            ((uint32_t)data[2] << 8) |
            ((uint32_t)data[1] << 16) |
            ((uint32_t)data[0] << 24); 
}

void u32_to_byte_array_little_endian(uint8_t* data, uint32_t length) {
    data[0] = length >> 24;
	data[1] = length >> 16;
	data[2] = length >> 8;
	data[3] = length;  
}

void u32_to_byte_array_big_endian(uint8_t* data, uint32_t length) {
    data[0] = length;
	data[1] = length >> 8;
	data[2] = length >> 16;
	data[3] = length >> 24;  
}