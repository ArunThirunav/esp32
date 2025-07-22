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
#include "fw_file_handler.h"
#include "utility.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

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
bool flag = true;

static uint8_t challenge_buf[32];
static int challenge_len = 0;
static bool is_authenticated = false;
unsigned char rsa_public_pem[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x50,
  0x55, 0x42, 0x4c, 0x49, 0x43, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d, 0x2d,
  0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x42, 0x49, 0x6a, 0x41, 0x4e, 0x42,
  0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47, 0x39, 0x77, 0x30, 0x42, 0x41,
  0x51, 0x45, 0x46, 0x41, 0x41, 0x4f, 0x43, 0x41, 0x51, 0x38, 0x41, 0x4d,
  0x49, 0x49, 0x42, 0x43, 0x67, 0x4b, 0x43, 0x41, 0x51, 0x45, 0x41, 0x30,
  0x58, 0x5a, 0x77, 0x59, 0x2f, 0x71, 0x58, 0x54, 0x4b, 0x79, 0x59, 0x44,
  0x69, 0x65, 0x36, 0x78, 0x2f, 0x74, 0x4a, 0x0a, 0x2b, 0x35, 0x39, 0x47,
  0x75, 0x4d, 0x50, 0x6d, 0x5a, 0x6b, 0x66, 0x50, 0x52, 0x6c, 0x4d, 0x33,
  0x4e, 0x73, 0x56, 0x6d, 0x4b, 0x4c, 0x55, 0x35, 0x6e, 0x4e, 0x52, 0x6e,
  0x78, 0x42, 0x54, 0x78, 0x6b, 0x52, 0x6e, 0x33, 0x58, 0x45, 0x39, 0x35,
  0x34, 0x4c, 0x73, 0x7a, 0x68, 0x6b, 0x6a, 0x4a, 0x5a, 0x4c, 0x56, 0x79,
  0x33, 0x50, 0x71, 0x6b, 0x37, 0x54, 0x46, 0x49, 0x46, 0x2b, 0x2f, 0x64,
  0x0a, 0x70, 0x52, 0x57, 0x61, 0x67, 0x4d, 0x2f, 0x62, 0x4b, 0x33, 0x63,
  0x73, 0x57, 0x56, 0x5a, 0x65, 0x38, 0x4d, 0x4b, 0x36, 0x66, 0x6a, 0x48,
  0x38, 0x51, 0x4b, 0x2f, 0x72, 0x36, 0x37, 0x62, 0x2b, 0x53, 0x6e, 0x6f,
  0x53, 0x36, 0x38, 0x6f, 0x38, 0x4d, 0x74, 0x66, 0x79, 0x6a, 0x43, 0x52,
  0x6b, 0x6c, 0x77, 0x63, 0x38, 0x32, 0x30, 0x48, 0x55, 0x68, 0x44, 0x6d,
  0x42, 0x44, 0x67, 0x77, 0x6e, 0x0a, 0x63, 0x38, 0x33, 0x41, 0x6d, 0x35,
  0x6d, 0x6b, 0x76, 0x61, 0x6f, 0x33, 0x6f, 0x38, 0x74, 0x5a, 0x59, 0x4b,
  0x4f, 0x44, 0x43, 0x71, 0x46, 0x48, 0x52, 0x47, 0x73, 0x48, 0x5a, 0x55,
  0x50, 0x66, 0x43, 0x44, 0x6f, 0x55, 0x52, 0x67, 0x32, 0x51, 0x6e, 0x52,
  0x79, 0x65, 0x71, 0x58, 0x75, 0x5a, 0x45, 0x79, 0x71, 0x4c, 0x48, 0x31,
  0x43, 0x51, 0x35, 0x53, 0x55, 0x54, 0x62, 0x37, 0x68, 0x2b, 0x0a, 0x77,
  0x58, 0x58, 0x64, 0x79, 0x6a, 0x44, 0x59, 0x66, 0x38, 0x48, 0x67, 0x44,
  0x5a, 0x63, 0x72, 0x45, 0x6d, 0x79, 0x70, 0x6d, 0x2f, 0x62, 0x51, 0x6e,
  0x72, 0x4f, 0x65, 0x66, 0x53, 0x53, 0x2b, 0x67, 0x64, 0x38, 0x4c, 0x33,
  0x44, 0x45, 0x6b, 0x77, 0x55, 0x2f, 0x6f, 0x48, 0x7a, 0x50, 0x77, 0x55,
  0x54, 0x36, 0x62, 0x6e, 0x79, 0x64, 0x70, 0x56, 0x63, 0x79, 0x53, 0x7a,
  0x64, 0x30, 0x65, 0x0a, 0x48, 0x55, 0x53, 0x53, 0x5a, 0x52, 0x35, 0x77,
  0x59, 0x53, 0x71, 0x5a, 0x34, 0x57, 0x45, 0x31, 0x44, 0x74, 0x4e, 0x55,
  0x51, 0x45, 0x6a, 0x55, 0x57, 0x5a, 0x46, 0x6c, 0x6b, 0x53, 0x66, 0x62,
  0x76, 0x69, 0x43, 0x72, 0x79, 0x64, 0x4e, 0x4d, 0x6b, 0x4c, 0x34, 0x67,
  0x48, 0x46, 0x66, 0x49, 0x59, 0x33, 0x35, 0x70, 0x5a, 0x39, 0x68, 0x70,
  0x53, 0x74, 0x53, 0x4e, 0x45, 0x37, 0x64, 0x36, 0x0a, 0x4d, 0x77, 0x49,
  0x44, 0x41, 0x51, 0x41, 0x42, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45,
  0x4e, 0x44, 0x20, 0x50, 0x55, 0x42, 0x4c, 0x49, 0x43, 0x20, 0x4b, 0x45,
  0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
};
unsigned int rsa_public_pem_len = 451;

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
	if(ctxt == NULL) {
		return BLE_ATT_ERR_INVALID_PDU;
	}
	if (is_authenticated == false)
	{
		/* ADD DISCONNECT HANDLER */
		return BLE_ATT_ERR_INVALID_HANDLE;
	}
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		return BLE_ATT_ERR_READ_NOT_PERMITTED;
		break;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		ble_request_handler(ctxt->om->om_data);
		break;		
	default:
		break;
	}
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
	if(ctxt == NULL) {
		return BLE_ATT_ERR_INVALID_PDU;
	}
	if (is_authenticated == false)
	{
		/* ADD DISCONNECT HANDLER */
		return BLE_ATT_ERR_INVALID_HANDLE;
	}
	switch (ctxt->op)
	{
	case BLE_GATT_ACCESS_OP_READ_CHR:
		return BLE_ATT_ERR_READ_NOT_PERMITTED;
		break;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		ble_request_handler(ctxt->om->om_data);
		notify_client(conn_handle, attr_handle, res);
		break;
	default:
		break;
	}
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
	if(ctxt == NULL) {
		return BLE_ATT_ERR_INVALID_PDU;
	}
	if (is_authenticated == false)
	{
		/* ADD DISCONNECT HANDLER */
		return BLE_ATT_ERR_INVALID_HANDLE;
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
int auth_cb(uint16_t conn_handle, uint16_t attr_handle,
							struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const char *desc = "File Transfer Charat";
	if(ctxt == NULL) {
		return BLE_ATT_ERR_INVALID_PDU;
	}
	
	switch (ctxt->op)
	{
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGI("AUTH: ", "Read Challenge Start");
		// Generate 16-byte challenge if not already done
		if (challenge_len == 0) {
			mbedtls_entropy_context entropy;
			mbedtls_ctr_drbg_context ctr_drbg;
			const char *pers = "rsa_challenge";

			mbedtls_entropy_init(&entropy);
			mbedtls_ctr_drbg_init(&ctr_drbg);
			mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
								(const unsigned char *)pers, strlen(pers));
			mbedtls_ctr_drbg_random(&ctr_drbg, challenge_buf, 16);
			challenge_len = 16;
		}
    	os_mbuf_append(ctxt->om, challenge_buf, challenge_len);
		ESP_LOGI("AUTH: ", "Read Challenge End");
		break;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		ESP_LOGI("AUTH: ", "Write Challenge Start");
		
		if (challenge_len == 0) return BLE_ATT_ERR_INVALID_HANDLE;
		// Load public key
		mbedtls_pk_context pk;
		mbedtls_pk_init(&pk);
		int rc = 0;
		rc = mbedtls_pk_parse_public_key(&pk, rsa_public_pem, strlen((const char *)rsa_public_pem) + 1);
		ESP_LOGI("AUTH: ", "Write Challenge %d", rc);
		// Hash challenge
		uint8_t hash[32];
		mbedtls_sha256(challenge_buf, challenge_len, hash, 0);
		ESP_LOGI("AUTH: ", "Write Challenge Hash Calculates");

		// Verify signature
		if (mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 0,
							ctxt->om->om_data, ctxt->om->om_len) == 0) {
			is_authenticated = true;
			notify_client(conn_handle, attr_handle, BLE_ACK);
			ESP_LOGI("AUTH: ", "Device authenticated.\n");
			challenge_len = 0;
			return 0;
		}
		else {
			notify_client(conn_handle, attr_handle, BLE_ACK);
			ESP_LOGE("AUTH: ", "Signature invalid.\n");
		}
		break;
	default:
		break;
	}
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