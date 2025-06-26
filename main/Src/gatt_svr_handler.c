#include "stdint.h"
#include <inttypes.h>
#include "gatt_svr_handler.h"
#include "uart_handler.h"
#include "config_file_handle.h"

static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);
volatile bool response_flag = false;
volatile bool config_request_flag = false;
uint8_t version_request[42] = {
	0xA5,
	0x53,
	0x00,
	0x00,
	0x00,
	0x20,
	0x00,
	0x00,
	0x01,
	0x00, /* MRA */
	0x00,
	0x00,
	0x01,
	0x00, /* MRB */
	0x00,
	0x00,
	0x01,
	0x00, /* CTA */
	0x00,
	0x00,
	0x01,
	0x00, /* CTB */
	0x00,
	0x00,
	0x01,
	0x00, /* COP*/
	0x00,
	0x00,
	0x01,
	0x00, /* MRUI */
	0x00,
	0x00,
	0x01,
	0x00, /* CTUI */
	0x00,
	0x00,
	0x01,
	0x00, /* COPUI */
	0xD3,
	0xA2,
	0x1A,
	0x71,
};

int file_transfer_write_cb(uint16_t conn_handle, uint16_t attr_handle,
						   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	// Simulate writing to file
	ESP_LOGI("BLE", "Received chunk of bytes");
	return 0;
}

int config_file_read_cb(uint16_t conn_handle, uint16_t attr_handle,
						struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	uint32_t length = 0;
	if (config_request_flag == false)
	{
		notify_client(conn_handle, attr_handle, REQUEST_PACKET_NOT_RECEIVED);
		return 0;
	}

	const uint8_t *temp = get_config_file(&length);

	ESP_LOGI("BLE", "Read Callback bytes: %ld", length);
	os_mbuf_append(ctxt->om, temp, length);
	/* 500 Bytes for Config File and 10 bytes incl. start, packType, len and crc*/
	if (length < 510)
	{
		config_request_flag = false;
		notify_client(conn_handle, attr_handle, BLE_ACK);
	}
	return 0;
}

int request_response_cb(uint16_t conn_handle, uint16_t attr_handle,
						struct ble_gatt_access_ctxt *ctxt, void *arg)
{

	error_code_t status = SUCCESS;

	const uint8_t *data = ctxt->om->om_data;

	/* CHANGE BASED ON READ WRITE */
	switch (ctxt->op)
	{
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGI("BLE", "Read Response");
		if (response_flag == true)
		{
			response_flag = false;
			os_mbuf_append(ctxt->om, version_request, sizeof(version_request));
		}
		else
		{
			notify_client(conn_handle, attr_handle, REQUEST_PACKET_NOT_RECEIVED);
		}
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

static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code)
{
	int rt = err_code;
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&rt, 1);
	ble_gattc_notify_custom(conn_handle, attr_handle, om);
}