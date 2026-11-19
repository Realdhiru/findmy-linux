#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BLE_ADDR_LEN 6
#define FINDMY_ADV_PAYLOAD_LEN 31
#define FINDMY_PUBKEY_LEN 28

bool findmy_build_payload(const uint8_t *pubkey, uint8_t *out_addr, uint8_t *out_payload);
int findmy_base64_decode(const char *in, size_t in_len, uint8_t *out, size_t out_max);
void findmy_base64_encode(const uint8_t *in, size_t len, char *out);
