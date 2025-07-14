/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */

#include "esp_log.h"
#include "nvs_flash.h"

#include "uart_handler.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "ble_app.h"
#include "flash_handler.h"
#include "fw_file_handler.h"

#define BLE_TASK_STACK_SIZE (4 * 1024)
#define UART_TASK_STACK_SIZE (10 * 1024)

extern QueueHandle_t fw_file_data_queue;
extern TaskHandle_t fw_file_write_handle;


static StackType_t ble_task_stack[BLE_TASK_STACK_SIZE];
static StackType_t uart_task_stack[UART_TASK_STACK_SIZE];

static StaticTask_t ble_task_tcb;
static StaticTask_t uart_task_tcb;

/**
 * @brief Host task for NimBLE stack execution.
 *
 * This FreeRTOS task runs the  host stack event loop. 
 * It is responsible for processing BLE events such as connections, 
 * disconnections, GATT operations, and GAP events.
 *
 * This task typically runs indefinitely and should be created 
 * during BLE initialization.
 *
 * @param param   Optional parameter passed to the task (usually NULL).
 *
 * @return None. This function never returns under normal operation.
 */
void host_task(void *param) {
	/* This function will return only when nimble_port_stop() is executed */
	nimble_port_run();
	nimble_port_freertos_deinit();
}

/**
 * @brief Main application entry point.
 *
 * This is the main entry function for the application. It initializes 
 * system peripherals, BLE stack, UART, security configurations, GATT 
 * server, and starts BLE advertising. It also creates necessary tasks 
 * like BLE host task and UART event handling task.
 *
 * @note This function is automatically called by the ESP-IDF framework 
 * after startup. It should never return.
 *
 * @return None.
 */
void app_main(void) {
	int32_t rc;

	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(nimble_port_init());

	ESP_ERROR_CHECK(ble_initialization());
	ESP_ERROR_CHECK(uart_initialization());
	ESP_ERROR_CHECK(flash_initalization());

	xTaskCreateStatic(uart_event_task, "uart_event_task", UART_TASK_STACK_SIZE, NULL, 3, uart_task_stack, &uart_task_tcb);
	ESP_LOGI("HEAP", "Free heap: %ld", esp_get_free_heap_size());
	fw_file_write_handle = xTaskCreateStaticPinnedToCore(ble_data_write_task, "ble_data_write_task", BLE_TASK_STACK_SIZE, NULL, 5, ble_task_stack, &ble_task_tcb, 1);
	nimble_port_freertos_init(host_task);
}
	