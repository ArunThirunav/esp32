#include "stdint.h"
#include "gatt_svr_handler.h"
#include "uart_handler.h"
#include "crc.h"

static void notify_error(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);

int file_transfer_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	// Simulate writing to file
	ESP_LOGI("BLE", "Received chunk of bytes");
	return 0;
}

int config_file_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	// Simulate writing to file
	ESP_LOGI("BLE", "Read Callback bytes");
	const char *resp = "Hello from ESP32!";
    os_mbuf_append(ctxt->om, resp, strlen(resp));
	return 0;
}

int request_response_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
	
	const uint8_t *data = ctxt->om->om_data;
	int32_t status = ESP_OK;
	/* CHANGE BASED ON READ WRITE */					
	switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_CHR:
			ESP_LOGI("BLE", "Read Response");
			break;
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			ESP_LOGI("BLE", "Request Sent");
			if(true != validate_crc(data)) {
				ESP_LOGW("TAG", "Invalid start byte: 0x%02X", data[0]);
				notify_error(conn_handle, attr_handle, CRC_CHECK_ERROR);
				return -1;
			}
			if (data[0] != START_BYTE) {
				ESP_LOGW("TAG", "Invalid start byte: 0x%02X", data[0]);
				notify_error(conn_handle, attr_handle, START_BYTE_ERROR);
				return -1;
			}
			status = parse_data(data);
			if(status < ESP_OK) {
				notify_error(conn_handle, attr_handle, INVALID_PACKET_ERROR);
			}			
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

static void notify_error(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code) {
	int rt = err_code;
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&rt, 1);
	ble_gattc_notify_custom(conn_handle, attr_handle, om);
}