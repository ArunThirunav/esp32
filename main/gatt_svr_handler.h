#ifndef GATT_SVR_HANDLER_H
#define GATT_SVR_HANDLER_H


#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int file_transfer_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

int file_descriptor_read_cb(uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt *ctxt, void *arg);

int config_file_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

int request_response_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* END OF GATT_SVR_HANDLER_H */