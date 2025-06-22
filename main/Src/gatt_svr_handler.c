#include "stdint.h"
#include <inttypes.h>
#include "gatt_svr_handler.h"
#include "uart_handler.h"

static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);

int file_transfer_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	// Simulate writing to file
	ESP_LOGI("BLE", "Received chunk of bytes");
	return 0;
}

int config_file_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	// Simulate writing to file
	int length = 0;
	const uint8_t *resp = uart_read_data(&length);
	ESP_LOGI("BLE", "Read Callback bytes: %d", length);
    os_mbuf_append(ctxt->om, resp, length);
	if (length < 500) {
		notify_client(conn_handle, attr_handle, BLE_ACK);
	}
	return 0;
}

int request_response_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	
	error_code_t status = SUCCESS;

	const uint8_t *data = ctxt->om->om_data;

	/* CHANGE BASED ON READ WRITE */					
	switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_CHR:
			ESP_LOGI("BLE", "Read Response");
			break;
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			status = uart_data_handler(data);
			notify_client(conn_handle, attr_handle, status);		
			break;
		default:
			break;
	}
	return 0;
}

int file_descriptor_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const char *desc = "File Transfer Charat";
	ESP_LOGI("BLE", "Description Called");
	os_mbuf_append(ctxt->om, desc, strlen(desc));
	return 0;
}

static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code) {
	int rt = err_code;
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&rt, 1);
	ble_gattc_notify_custom(conn_handle, attr_handle, om);
}