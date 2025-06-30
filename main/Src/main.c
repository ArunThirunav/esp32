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
/* BLE */
#include "gatt_svr.h"
#include "console/console.h"
#include "driver/gpio.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "uart_handler.h"

static const char *tag = "NimBLE_BLE_PRPH";
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static void security_initialization(void);
static uint8_t own_addr_type;

void ble_store_config_init(void);

/**
 * @brief Starts BLE advertising.
 *
 * This function configures and starts BLE advertising. It sets up the 
 * advertisement data (device name, services, etc.) and makes the device 
 * discoverable and connectable to BLE clients.
 *
 * @note This should be called after the BLE stack is synchronized and 
 * the GATT server has been initialized.
 *
 * @return None
 */
static void ble_advertisement(void) {
	struct ble_gap_adv_params adv_params;
	struct ble_hs_adv_fields fields;
	const char *name;
	int rc;

	memset(&fields, 0, sizeof fields);

	/* Advertise two flags:
	 *     o Discoverability in forthcoming advertisement (general)
	 *     o BLE-only (BR/EDR unsupported).
	 */
	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

	/* Indicate that the TX power level field should be included; have the
	 * stack fill this value automatically.  This is done by assigning the
	 * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
	 */
	name = ble_svc_gap_device_name();
	fields.name = (uint8_t *)name;
	fields.name_len = strlen(name);
	fields.name_is_complete = 1;

	rc = ble_gap_adv_set_fields(&fields);
	if (rc != 0)
	{
		MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
		return;
	}

	/* Begin advertising. */
	memset(&adv_params, 0, sizeof adv_params);
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
						   ble_gap_event, NULL);
	if (rc != 0)
	{
		MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
		return;
	}
}

/**
 * @brief The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
	struct ble_gap_conn_desc desc;
	int rc;

	switch (event->type)
	{
	case BLE_GAP_EVENT_LINK_ESTAB:
		printf("BLE CONNECTION ESTABLISHED\n");
		/* A new connection was established or a connection attempt failed. */
		MODLOG_DFLT(INFO, "connection %s; status=%d ",
					event->link_estab.status == 0 ? "established" : "failed",
					event->link_estab.status);
		if (event->link_estab.status == 0)
		{
			rc = ble_gap_conn_find(event->link_estab.conn_handle, &desc);
			assert(rc == 0);
		}
		MODLOG_DFLT(INFO, "\n");

		if (event->link_estab.status != 0)
		{
			/* Connection failed; resume advertising. */
			ble_advertisement();
		}
		return 0;

	case BLE_GAP_EVENT_DISCONNECT:
		MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
		MODLOG_DFLT(INFO, "\n");

		/* Connection terminated; resume advertising. */
		ble_advertisement();
		return 0;

	case BLE_GAP_EVENT_CONN_UPDATE:
		/* The central has updated the connection parameters. */
		MODLOG_DFLT(INFO, "connection updated; status=%d ",
					event->conn_update.status);
		rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
		assert(rc == 0);
		MODLOG_DFLT(INFO, "\n");
		return 0;

	case BLE_GAP_EVENT_ADV_COMPLETE:
		MODLOG_DFLT(INFO, "advertise complete; reason=%d",
					event->adv_complete.reason);
		ble_advertisement();
		return 0;

	case BLE_GAP_EVENT_ENC_CHANGE:
		/* Encryption has been enabled or disabled for this connection. */
		MODLOG_DFLT(INFO, "encryption change event; status=%d ",
					event->enc_change.status);
		rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
		assert(rc == 0);
		MODLOG_DFLT(INFO, "\n");
		return 0;

	case BLE_GAP_EVENT_NOTIFY_TX:
		MODLOG_DFLT(INFO,
					"notify_tx event; conn_handle=%d attr_handle=%d "
					"status=%d is_indication=%d",
					event->notify_tx.conn_handle, event->notify_tx.attr_handle,
					event->notify_tx.status, event->notify_tx.indication);
		return 0;

	case BLE_GAP_EVENT_SUBSCRIBE:
		MODLOG_DFLT(INFO,
					"subscribe event; conn_handle=%d attr_handle=%d "
					"reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
					event->subscribe.conn_handle, event->subscribe.attr_handle,
					event->subscribe.reason, event->subscribe.prev_notify,
					event->subscribe.cur_notify, event->subscribe.prev_indicate,
					event->subscribe.cur_indicate);
		return 0;

	case BLE_GAP_EVENT_MTU:
		MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
					event->mtu.conn_handle, event->mtu.channel_id,
					event->mtu.value);
		return 0;

	case BLE_GAP_EVENT_REPEAT_PAIRING:
		/* We already have a bond with the peer, but it is attempting to
		 * establish a new secure link.  This app sacrifices security for
		 * convenience: just throw away the old bond and accept the new link.
		 */

		/* Delete the old bond. */
		rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
		assert(rc == 0);
		ble_store_util_delete_peer(&desc.peer_id_addr);

		/* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
		 * continue with the pairing operation.
		 */
		return BLE_GAP_REPEAT_PAIRING_RETRY;

	case BLE_GAP_EVENT_PASSKEY_ACTION:
		ESP_LOGI(tag, "PASSKEY_ACTION_EVENT started");
		return 0;

	case BLE_GAP_EVENT_AUTHORIZE:
		MODLOG_DFLT(INFO,
					"authorize event: conn_handle=%d attr_handle=%d is_read=%d",
					event->authorize.conn_handle, event->authorize.attr_handle,
					event->authorize.is_read);

		/* The default behaviour for the event is to reject authorize request */
		event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
		return 0;
	}
	return 0;
}

/**
 * @brief Callback invoked when the BLE host stack resets.
 *
 * This function is called when the BLE host encounters a fatal error 
 * and performs a reset. It is useful for logging the reason and 
 * potentially triggering corrective actions or a full system reset.
 *
 * @param reason   Reset reason code. Matches one of the BLE host error codes.
 *
 * @return         None
 */
static void ble_on_reset(int reason) {
	MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

/**
 * @brief Callback invoked when the BLE stack is synchronized.
 *
 * This function is called when the NimBLE host and controller are 
 * fully synchronized and the BLE stack is ready to begin operations, 
 * such as advertising or scanning. Typically, GATT server initialization 
 * and advertising start from this function.
 *
 * @return None
 */
static void ble_on_sync(void) {
	int rc;
	/* Make sure we have proper identity address set (public preferred) */
	rc = ble_hs_util_ensure_addr(0);
	assert(rc == 0);

	/* Figure out address to use while advertising (no privacy for now) */
	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0)
	{
		MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
		return;
	}

	/* Printing ADDR */
	uint8_t addr_val[6] = {0};
	rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

	MODLOG_DFLT(INFO, "Device Address: ");
	print_addr(addr_val);
	MODLOG_DFLT(INFO, "\n");
	/* Begin advertising. */
	ble_advertisement();
}

/**
 * @brief BLE host task for NimBLE stack execution.
 *
 * This FreeRTOS task runs the NimBLE BLE host stack event loop. 
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
void ble_host_task(void *param)
{
	printf("BLE Host Task Started");
	/* This function will return only when nimble_port_stop() is executed */
	nimble_port_run();
	printf("BLE After Run");
	nimble_port_freertos_deinit();
	printf("BLE After De init");
}

/**
 * @brief Initializes BLE security configuration.
 *
 * This function configures the BLE security settings, including 
 * bonding, authentication requirements, I/O capabilities, and 
 * key distribution. It ensures that BLE connections are secure 
 * with the desired pairing and bonding modes.
 *
 * @note This should be called after BLE stack initialization and 
 * before starting advertising or accepting connections.
 *
 * @return None.
 */
static void security_initialization(void) {
    ble_hs_cfg.sm_io_cap = 3;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 0;
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
	int rc;

	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ret = nimble_port_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(tag, "Failed to init nimble %d ", ret);
		return;
	}
	/* Initialize the NimBLE host configuration. */
	ble_hs_cfg.reset_cb = ble_on_reset;
	ble_hs_cfg.sync_cb = ble_on_sync;
	ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	security_initialization();

	rc = gatt_svr_init();
	assert(rc == 0);

	/* Set the default device name. */
	rc = ble_svc_gap_device_name_set("CT Car 3");
	assert(rc == 0);

	/* XXX Need to have template for store */
	ble_store_config_init();

	uart_initialization();
	// Create a task to handler UART event from ISR
	xTaskCreate(uart_event_task, "uart_event_task", 3 * 1024, NULL, 3, NULL);

	nimble_port_freertos_init(ble_host_task);
}
