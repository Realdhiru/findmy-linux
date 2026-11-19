#include "hci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 31
#endif

#ifndef BTPROTO_HCI
#define BTPROTO_HCI 1
#endif

#define HCI_COMMAND_PKT 0x01
#define HCI_EVENT_PKT   0x04
#define EVT_CMD_COMPLETE 0x0e

#define HCIDEVUP	_IOW('H', 201, int)

struct sockaddr_hci {
    sa_family_t hci_family;
    uint16_t    hci_dev;
    uint16_t    hci_channel;
};

int hci_open_device(int dev_id) {
    int fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (fd < 0) {
        return -errno;
    }

    // Automatically bring interface UP if it is DOWN (prevents error -100 ENETDOWN)
    if (ioctl(fd, HCIDEVUP, dev_id) < 0) {
        if (errno != EALREADY && errno != EBUSY) {
            fprintf(stderr, "Note: Bringing up hci%d via ioctl returned: %s\n", dev_id, strerror(errno));
        }
    }

    struct sockaddr_hci addr;
    memset(&addr, 0, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = (uint16_t)dev_id;
    addr.hci_channel = 0; // HCI_CHANNEL_RAW

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        int err = errno;
        close(fd);
        return -err;
    }

    return fd;
}

int hci_send_cmd(int fd, uint16_t opcode, const void *param, uint8_t param_len) {
    uint8_t buf[256];
    buf[0] = HCI_COMMAND_PKT;
    buf[1] = (uint8_t)(opcode & 0xff);
    buf[2] = (uint8_t)(opcode >> 8);
    buf[3] = param_len;
    if (param && param_len > 0) {
        memcpy(&buf[4], param, param_len);
    }

    size_t total_len = 4 + param_len;
    if (write(fd, buf, total_len) < 0) {
        return -errno;
    }

    uint8_t resp[256];
    ssize_t n = read(fd, resp, sizeof(resp));
    if (n < 0) {
        return -errno;
    }

    if (n >= 6 && resp[0] == HCI_EVENT_PKT && resp[1] == EVT_CMD_COMPLETE) {
        uint8_t status = resp[5];
        if (status != 0) {
            return -status;
        }
    }

    return 0;
}

int hci_le_set_random_address(int fd, const uint8_t addr[6]) {
    uint16_t opcode = HCI_OPCODE(OGF_LE_CTL, OCF_LE_SET_RAND_ADDR);
    return hci_send_cmd(fd, opcode, addr, 6);
}

int hci_le_set_adv_parameters(int fd, uint16_t min_interval_ms, uint16_t max_interval_ms) {
    uint16_t min_slots = (uint16_t)((min_interval_ms * 16) / 10);
    uint16_t max_slots = (uint16_t)((max_interval_ms * 16) / 10);

    uint8_t params[15];
    memset(params, 0, sizeof(params));

    params[0] = (uint8_t)(min_slots & 0xff);
    params[1] = (uint8_t)(min_slots >> 8);
    params[2] = (uint8_t)(max_slots & 0xff);
    params[3] = (uint8_t)(max_slots >> 8);
    params[4] = 0x03; // ADV_NONCONN_IND
    params[5] = 0x01; // Random Device Address
    params[6] = 0x00; // Public
    params[13] = 0x07; // Channels 37, 38, 39
    params[14] = 0x00;

    uint16_t opcode = HCI_OPCODE(OGF_LE_CTL, OCF_LE_SET_ADV_PARAM);
    return hci_send_cmd(fd, opcode, params, sizeof(params));
}

int hci_le_set_adv_data(int fd, const uint8_t *data, uint8_t len) {
    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));
    buf[0] = len;
    if (len > 31) len = 31;
    memcpy(&buf[1], data, len);

    uint16_t opcode = HCI_OPCODE(OGF_LE_CTL, OCF_LE_SET_ADV_DATA);
    return hci_send_cmd(fd, opcode, buf, sizeof(buf));
}

int hci_le_set_adv_enable(int fd, bool enable) {
    uint8_t param = enable ? 0x01 : 0x00;
    uint16_t opcode = HCI_OPCODE(OGF_LE_CTL, OCF_LE_SET_ADV_EN);
    return hci_send_cmd(fd, opcode, &param, 1);
}
