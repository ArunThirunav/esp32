/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : gatt_svr_handler.c
 * @brief          : Handle the GATT service and it's callbacks
 ******************************************************************************
 */
/* USER CODE END Header */
#include "stdint.h"
#include <inttypes.h>
#include "gatt_svr_handler.h"
#include "uart_handler.h"
#include "config_file_handle.h"

static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);
volatile bool response_flag = false;
volatile bool config_request_flag = false;
uint8_t version_request[42] = { 0xA5,
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

/**
 * @brief GATT Write callback for file transfer characteristic.
 *
 * This callback is invoked when a BLE client writes data to the file 
 * transfer characteristic. It receives chunks of data that can be 
 * assembled to reconstruct the file on the server side.
 *
 * @param conn_handle    Connection handle identifying the BLE connection.
 * @param attr_handle    Attribute handle of the characteristic being written.
 * @param ctxt           Pointer to the BLE GATT access context. Contains 
 *                       the operation type, data buffer, and other metadata.
 * @param arg            Optional user-defined argument passed during service 
 *                       registration (can be NULL or a user context).
 *
 * @return               0 on success (BLE_ATT_ERR_SUCCESS), or an appropriate 
 *                       BLE ATT error code on failure.
 */
int file_transfer_write_cb(uint16_t conn_handle, uint16_t attr_handle,
						   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	// Simulate writing to file
	ESP_LOGI("BLE", "Received chunk of bytes");
	return 0;
}

/**
 * @brief GATT Read callback for the configuration file characteristic.
 *
 * This callback is invoked when a BLE client performs a read operation on 
 * the configuration file characteristic. The function provides the configuration 
 * data to the client in response to the read request.
 *
 * @param conn_handle    Connection handle identifying the BLE connection.
 * @param attr_handle    Attribute handle of the characteristic being read.
 * @param ctxt           Pointer to the BLE GATT access context. Contains 
 *                       the operation type and the response buffer where 
 *                       the server should append the data to send.
 * @param arg            Optional user-defined argument passed during service 
 *                       registration (can be NULL or a user context pointer).
 *
 * @return               0 on success (BLE_ATT_ERR_SUCCESS), or an appropriate 
 *                       BLE ATT error code on failure.
 */
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

/**
 * @brief GATT Read callback for request-response characteristic.
 *
 * This callback is invoked when a BLE client performs a read operation on 
 * the request-response characteristic. The server generates a response 
 * dynamically based on the current state, a prior write request, or internal logic.
 *
 * @param conn_handle    Connection handle identifying the BLE connection.
 * @param attr_handle    Attribute handle of the characteristic being accessed.
 * @param ctxt           Pointer to the BLE GATT access context. Contains 
 *                       the operation type and the response buffer to append 
 *                       response data to.
 * @param arg            Optional user-defined argument passed during service 
 *                       or characteristic registration (can be NULL or a user context).
 *
 * @return               0 on success (BLE_ATT_ERR_SUCCESS), or an appropriate 
 *                       BLE ATT error code on failure.
 */
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
		response_flag = false;
		os_mbuf_append(ctxt->om, version_request, sizeof(version_request));
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

/**
 * @brief GATT Read callback for the file descriptor characteristic.
 *
 * This callback is invoked when a BLE client performs a read operation on 
 * the file descriptor characteristic. It returns metadata about the file, 
 * such as file size, CRC, file type, version, or any descriptor information 
 * required before initiating a file transfer.
 *
 * @param conn_handle    Connection handle identifying the BLE connection.
 * @param attr_handle    Attribute handle of the characteristic being read.
 * @param ctxt           Pointer to the BLE GATT access context. Contains 
 *                       the operation type and the response buffer where 
 *                       the server should append the descriptor data.
 * @param arg            Optional user-defined argument passed during service 
 *                       or characteristic registration (can be NULL or a context pointer).
 *
 * @return               0 on success (BLE_ATT_ERR_SUCCESS), or an appropriate 
 *                       BLE ATT error code on failure.
 */
int file_descriptor_read_cb(uint16_t conn_handle, uint16_t attr_handle,
							struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const char *desc = "File Transfer Charat";
	ESP_LOGI("BLE", "Description Called");
	os_mbuf_append(ctxt->om, desc, strlen(desc));
	return 0;
}

/**
 * @brief Sends a BLE notification to the client with an error or status code.
 *
 * This function sends a notification to the connected BLE client on the 
 * specified characteristic. It is typically used to inform the client about 
 * asynchronous events, transfer status, errors, or completion statuses 
 * during operations like file transfer or command execution.
 *
 * @param conn_handle    Connection handle identifying the BLE connection.
 * @param attr_handle    Attribute handle of the characteristic used for notification.
 * @param err_code       Error or status code to notify the client.
 *
 * @return               None
 */
static void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code)
{
	int rt = err_code;
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&rt, 1);
	ble_gattc_notify_custom(conn_handle, attr_handle, om);
}