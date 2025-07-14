#include "flash_handler.h"
#include "esp_log.h"

static esp_vfs_littlefs_conf_t config;


int32_t flash_initalization(void) {   
    config.base_path = "/storage";
    config.format_if_mount_failed = true;
    config.partition_label = "storage";
    config.dont_mount = false;

    esp_err_t result = esp_vfs_littlefs_register(&config);
    ESP_LOGI("FLASH: ", "%d", result);
    return (int32_t)result;
}

uint32_t get_total_flash_size(uint32_t* total, uint32_t* used) {
    size_t* l_total = (size_t*)total;
    size_t* l_used = (size_t*)used;
    esp_err_t result = esp_littlefs_info(config.partition_label, l_total, l_used);
    *total = *l_total;
    *used = *l_used;
    // ESP_LOGI("FLASH SIZE: ", "Total: %ld, Used: %ld", *total, *used);

    return (int32_t)result;
}

int32_t write_data_to_flash(const uint8_t* data, uint32_t len) {
    FILE* fs = NULL;
    uint32_t total = 0;
    uint32_t used = 0;
    get_total_flash_size(&total, &used);
    fs = fopen("/storage/nexus_fw.bin", "ab");
    if(fs == NULL) {
        ESP_LOGE("FLASH HANDLER: ", "Error in Writing");
        return -1;
    }
    if(used < total) {
        fwrite(data, sizeof(uint8_t), len, fs);
    }
    fflush(fs);  // optional but safer
    fclose(fs);
    return 0;
}
    
int32_t read_data_from_flash(uint8_t* data, uint32_t len) {
    FILE* fs = NULL;
    fs = fopen("/storage/nexus_fw.bin", "rb"); 
    if(fs == NULL) {
        ESP_LOGE("FLASH HANDLER: ", "Error in Reading");
        return -1;
    }
    size_t bytes_read;    
    bytes_read = fread(data, sizeof(uint8_t), len, fs);
    fclose(fs);
    return 0;
}

int32_t format_flash(void) {
    uint32_t total, used;
    esp_err_t result = ESP_OK;
    get_total_flash_size(&total, &used);
    ESP_LOGI("FLASH: ", "Before Formating: Total: %ld, Used: %ld", total, used);
    result = esp_littlefs_format(config.partition_label);
    if (result != ESP_OK){
        ESP_LOGE("FLASH HANDLER: ", "Formating Failed...");
    }    
    get_total_flash_size(&total, &used);
    ESP_LOGI("FLASH: ", "After Formating: Total: %ld, Used: %ld", total, used);
    return (int32_t)result;
}
