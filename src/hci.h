#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * HCI Opcodes & Definitions (Bluetooth Core Specification)
 */
#define OGF_LE_CTL           0x08
#define OCF_LE_SET_RAND_ADDR 0x0005
#define OCF_LE_SET_ADV_PARAM 0x0006
#define OCF_LE_SET_ADV_DATA  0x0008
#define OCF_LE_SET_ADV_EN    0x000A

#define HCI_OPCODE(ogf, ocf) ((uint16_t)(((ocf) & 0x03ff) | (((ogf) & 0x3f) << 10)))

/*
 * Open raw HCI socket to the specified adapter (e.g. hci0 -> dev_id = 0).
 * Returns socket fd, or negative errno on error.
 */
int hci_open_device(int dev_id);

/*
 * Send raw HCI command packet and verify command completion status.
 */
int hci_send_cmd(int fd, uint16_t opcode, const void *param, uint8_t param_len);

/*
 * High-level BLE advertisement configuration routines
 */
int hci_le_set_random_address(int fd, const uint8_t addr[6]);
int hci_le_set_adv_parameters(int fd, uint16_t min_interval_ms, uint16_t max_interval_ms);
int hci_le_set_adv_data(int fd, const uint8_t *data, uint8_t len);
int hci_le_set_adv_enable(int fd, bool enable);
