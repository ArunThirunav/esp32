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
#include "ble_request_handler.h"
#include "utility.h"

/* DEFINES */

void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code);
uint8_t version_request[42] = {
    0xA5, 0x53, 0x00, 0x00, 0x00, 0x20, 						/* START_BYTE, PACKET_TYPE, LENGTH */
	0x00, 0x00, 0x01, 0x00, 									/* MRA */
    0x00, 0x00, 0x01, 0x00,                                     /* MRB */
    0x00, 0x00, 0x01, 0x00,                                     /* CTA */
    0x00, 0x00, 0x01, 0x00,                                     /* CTB */
    0x00, 0x00, 0x01, 0x00,                                     /* COP*/
    0x00, 0x00, 0x01, 0x00,                                     /* MRUI */
    0x00, 0x00, 0x01, 0x00,                                     /* CTUI */
    0x00, 0x00, 0x01, 0x00,                                     /* COPUI */
    0xD3, 0xA2, 0x1A, 0x71,
};
int counter = 0;
uint8_t res = 0xAA;
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
	/* TODO: HANDLE THE FW CHUNK SAVE TO FLASH */
	ESP_LOGI("BLE", "Received chunk of bytes: %d", ctxt->om->om_len );
	ESP_LOGI("BLE", "Total bytes: %d", counter +=ctxt->om->om_len );
	uint8_t tx, rx;
	ble_gap_read_le_phy(conn_handle, &tx, &rx);
	ESP_LOGI("BLE", "TX: %d, RX: %d", tx, rx);
	notify_client(conn_handle, attr_handle, res);
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
	if (ctxt->om->om_data == NULL)
	{
		ESP_LOGI("CONFIG_FILE_WRITE_CB: ", "NULL DATA");
		return 0;
	}
	
	switch (ctxt->op)
	{
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGI("BLE", "Read Response");
		os_mbuf_append(ctxt->om, version_request, sizeof(version_request));
		break;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		ESP_LOGI("BLE", "Write Response: %d", ctxt->om->om_len);
		ESP_LOGI("BLE", "Write Response: %s", ctxt->om->om_data);
		ble_request_handler(ctxt->om->om_data);
		notify_client(conn_handle, attr_handle, res);
		break;
	default:
		break;
	}
	return 0;
	ESP_LOGI("WRITE CONFIG: ", "CHARACTERISTICS");
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

	error_code_t status = BLE_ACK;
	if (ctxt->om->om_data == NULL)
	{
		ESP_LOGI("ERROR: ", "NULL DATA");
		return 0;
	}
	const uint8_t *data = ctxt->om->om_data;
	/* CHANGE BASED ON READ WRITE */
	switch (ctxt->op)
	{
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGI("BLE", "Read Response");
		os_mbuf_append(ctxt->om, version_request, sizeof(version_request));
		break;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		status = ble_request_handler(data);
		ESP_LOGI("conn_handle", "%d", conn_handle);
		ESP_LOGI("attr_handle", "%d", attr_handle);
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
void notify_client(uint16_t conn_handle, uint16_t attr_handle, uint8_t err_code)
{
	int32_t rt = err_code;
	struct os_mbuf *om = ble_hs_mbuf_from_flat(&rt, 1);
	ble_gattc_notify_custom(conn_handle, attr_handle, om);
}