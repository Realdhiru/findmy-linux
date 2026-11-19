#include "beacon.h"
#include <string.h>

static const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void findmy_base64_encode(const uint8_t *in, size_t len, char *out) {
    size_t i = 0, j = 0;
    while (i < len) {
        size_t rem = len - i;
        uint32_t a = in[i++];
        uint32_t b = (rem > 1) ? in[i++] : 0;
        uint32_t c = (rem > 2) ? in[i++] : 0;

        uint32_t triple = (a << 16) | (b << 8) | c;

        out[j++] = b64_chars[(triple >> 18) & 0x3F];
        out[j++] = b64_chars[(triple >> 12) & 0x3F];
        out[j++] = (rem > 1) ? b64_chars[(triple >> 6) & 0x3F] : '=';
        out[j++] = (rem > 2) ? b64_chars[triple & 0x3F] : '=';
    }
    out[j] = '\0';
}

static const int b64_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
    ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63,
};

int findmy_base64_decode(const char *in, size_t in_len, uint8_t *out, size_t out_max) {
    size_t out_idx = 0;
    uint32_t val = 0;
    int bits = -8;

    for (size_t i = 0; i < in_len; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
        if (b64_table[c] == 0 && c != 'A') return -1;

        val = (val << 6) | (uint32_t)b64_table[c];
        bits += 6;

        if (bits >= 0) {
            if (out_idx >= out_max) return -1;
            out[out_idx++] = (uint8_t)((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    return (int)out_idx;
}

bool findmy_build_payload(const uint8_t *pubkey, uint8_t *out_addr, uint8_t *out_payload) {
    if (!pubkey || !out_addr || !out_payload) return false;

    uint8_t raw_addr[6];
    raw_addr[0] = (uint8_t)(pubkey[0] | 0b11000000);
    raw_addr[1] = pubkey[1];
    raw_addr[2] = pubkey[2];
    raw_addr[3] = pubkey[3];
    raw_addr[4] = pubkey[4];
    raw_addr[5] = pubkey[5];

    // Store in Little-Endian order for HCI LE_Set_Random_Address
    for (int i = 0; i < 6; i++) {
        out_addr[i] = raw_addr[5 - i];
    }

    memset(out_payload, 0, FINDMY_ADV_PAYLOAD_LEN);
    out_payload[0]  = 0x1e;        /* Length: 30 bytes following */
    out_payload[1]  = 0xff;        /* Type: Manufacturer Specific Data */
    out_payload[2]  = 0x4c;        /* Apple Company ID LSB (0x004c) */
    out_payload[3]  = 0x00;        /* Apple Company ID MSB */
    out_payload[4]  = 0x12;        /* Subtype: Find My Offline Finding */
    out_payload[5]  = 0x19;        /* Subtype Length: 25 bytes */
    out_payload[6]  = 0x00;        /* Status / State */

    memcpy(&out_payload[7], &pubkey[6], 22);
    out_payload[29] = (uint8_t)(pubkey[0] >> 6);
    out_payload[30] = 0x00;

    return true;
}
