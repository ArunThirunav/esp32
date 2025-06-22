/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "gatt_svr.h"
#include "gatt_svr_handler.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/ans/ble_svc_ans.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*** Maximum number of characteristics with the notify flag ***/
#define MAX_NOTIFY 5

static const ble_uuid128_t gatt_svr_svc_uuid =
    BLE_UUID128_INIT(0x32, 0x4F, 0xD3, 0xC6, 0x89, 0xB2, 0xBB, 0x94, 0xBE, 0x44,
                     0x84, 0xAD, 0x10, 0x00, 0xBD, 0xA2);

static const ble_uuid128_t gatt_file_write_uuid =
    BLE_UUID128_INIT(0x32, 0x4F, 0xD3, 0xC6, 0x89, 0xB2, 0xBB, 0x94, 0xBE, 0x44,
                     0x84, 0xAD, 0x11, 0x00, 0xBD, 0xA2);

static const ble_uuid128_t gatt_file_read_uuid =
    BLE_UUID128_INIT(0x32, 0x4F, 0xD3, 0xC6, 0x89, 0xB2, 0xBB, 0x94, 0xBE, 0x44,
                     0x84, 0xAD, 0x12, 0x00, 0xBD, 0xA2);

static const ble_uuid128_t gatt_misc_write_uuid =
    BLE_UUID128_INIT(0x32, 0x4F, 0xD3, 0xC6, 0x89, 0xB2, 0xBB, 0x94, 0xBE, 0x44,
                     0x84, 0xAD, 0x13, 0x00, 0xBD, 0xA2);

static const ble_uuid128_t gatt_future_uuid =
    BLE_UUID128_INIT(0x32, 0x4F, 0xD3, 0xC6, 0x89, 0xB2, 0xBB, 0x94, 0xBE, 0x44,
                     0x84, 0xAD, 0x14, 0x00, 0xBD, 0xA2);


/* A custom descriptor */
static uint8_t gatt_svr_dsc_val;

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
	{.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &gatt_svr_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
			{
				.uuid = &gatt_file_write_uuid.u,
				.access_cb = file_transfer_write_cb,
				.flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &gatt_file_read_uuid.u,
				.access_cb = config_file_read_cb,
				.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &gatt_misc_write_uuid.u,
				.access_cb = request_response_cb,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &gatt_future_uuid.u,
				.access_cb = file_transfer_write_cb,
				.flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_READ |
						BLE_GATT_CHR_F_NOTIFY,
			},
			{0},
		}
	},
    {0}
};

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
  char buf[BLE_UUID_STR_LEN];

  switch (ctxt->op) {
  case BLE_GATT_REGISTER_OP_SVC:
    MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                ctxt->svc.handle);
    break;

  case BLE_GATT_REGISTER_OP_CHR:
    MODLOG_DFLT(DEBUG,
                "registering characteristic %s with "
                "def_handle=%d val_handle=%d\n",
                ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                ctxt->chr.def_handle, ctxt->chr.val_handle);
    break;

  case BLE_GATT_REGISTER_OP_DSC:
    MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                ctxt->dsc.handle);
    break;

  default:
    assert(0);
    break;
  }
}

int gatt_svr_init(void) {
  int rc;

  ble_svc_gap_init();
  ble_svc_gatt_init();

  rc = ble_gatts_count_cfg(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  rc = ble_gatts_add_svcs(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  /* Setting a value for the read-only descriptor */
  gatt_svr_dsc_val = 0x99;

  return 0;
}
