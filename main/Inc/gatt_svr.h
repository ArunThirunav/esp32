/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : gatt_svr.h
 * @brief          : Header for gatt_svr.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */
#ifndef GATT_SVR_H
#define GATT_SVR_H

#include <stdbool.h>
#include <stdint.h>
#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "esp_peripheral.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int32_t gatt_svr_init(void);

#ifdef __cplusplus
}
#endif

#endif /* END OF GATT_SVR_H */
