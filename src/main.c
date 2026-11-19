#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "hci.h"
#include "beacon.h"

static volatile sig_atomic_t g_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n\n"
        "Universal Linux Apple Find My BLE Beacon Daemon\n\n"
        "Options:\n"
        "  -k, --key <base64>       28-byte base64 advertisement key\n"
        "  -f, --key-file <path>    Path to file containing base64 advertisement key\n"
        "  -i, --interface <id>     HCI interface index (default: 0 for hci0)\n"
        "  -t, --interval <ms>      Advertising interval in ms (default: 2000)\n"
        "  -d, --daemon             Run detached as a background daemon\n"
        "  -h, --help               Show this help message\n\n",
        prog);
}

int main(int argc, char *argv[]) {
    char *key_str = NULL;
    char *key_file = NULL;
    int hci_id = 0;
    int interval_ms = 2000;
    bool daemonize = false;

    static struct option long_options[] = {
        {"key",       required_argument, 0, 'k'},
        {"key-file",  required_argument, 0, 'f'},
        {"interface", required_argument, 0, 'i'},
        {"interval",  required_argument, 0, 't'},
        {"daemon",    no_argument,       0, 'd'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:f:i:t:dh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k':
                key_str = strdup(optarg);
                break;
            case 'f':
                key_file = strdup(optarg);
                break;
            case 'i':
                hci_id = atoi(optarg);
                break;
            case 't':
                interval_ms = atoi(optarg);
                if (interval_ms < 100) interval_ms = 100;
                break;
            case 'd':
                daemonize = true;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (key_file) {
        FILE *fp = fopen(key_file, "r");
        if (!fp) {
            perror("Failed to open key file");
            return 1;
        }
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            // strip newline
            line[strcspn(line, "\r\n")] = 0;
            key_str = strdup(line);
        }
        fclose(fp);
        free(key_file);
    }

    if (!key_str) {
        fprintf(stderr, "Error: Advertisement key is required. Specify with -k or -f.\n");
        print_usage(argv[0]);
        return 1;
    }

    uint8_t pubkey[64];
    int decoded_len = findmy_base64_decode(key_str, strlen(key_str), pubkey, sizeof(pubkey));
    free(key_str);

    if (decoded_len != FINDMY_PUBKEY_LEN) {
        fprintf(stderr, "Error: Invalid advertisement key length (decoded %d bytes, expected %d bytes)\n",
                decoded_len, FINDMY_PUBKEY_LEN);
        return 1;
    }

    uint8_t bd_addr[6];
    uint8_t payload[FINDMY_ADV_PAYLOAD_LEN];
    if (!findmy_build_payload(pubkey, bd_addr, payload)) {
        fprintf(stderr, "Error: Failed to construct Find My payload\n");
        return 1;
    }

    if (daemonize) {
        if (daemon(0, 0) < 0) {
            perror("daemon");
            return 1;
        }
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    int fd = hci_open_device(hci_id);
    if (fd < 0) {
        fprintf(stderr, "Error: Failed to open hci%d: %s (Are you running with CAP_NET_ADMIN / sudo?)\n",
                hci_id, strerror(-fd));
        return 1;
    }

    // Disable existing advertising first
    hci_le_set_adv_enable(fd, false);

    // Set static random address
    int rc = hci_le_set_random_address(fd, bd_addr);
    if (rc != 0) {
        fprintf(stderr, "Warning: Failed to set BLE random address (code: %d)\n", rc);
    }

    // Configure advertising parameters
    rc = hci_le_set_adv_parameters(fd, (uint16_t)interval_ms, (uint16_t)(interval_ms + 100));
    if (rc != 0) {
        fprintf(stderr, "Error: Failed to set advertising parameters (code: %d)\n", rc);
        close(fd);
        return 1;
    }

    // Load Find My GAP advertisement payload
    rc = hci_le_set_adv_data(fd, payload, FINDMY_ADV_PAYLOAD_LEN);
    if (rc != 0) {
        fprintf(stderr, "Error: Failed to load Find My advertising data (code: %d)\n", rc);
        close(fd);
        return 1;
    }

    // Enable advertising
    rc = hci_le_set_adv_enable(fd, true);
    if (rc != 0) {
        fprintf(stderr, "Error: Failed to enable BLE advertising (code: %d)\n", rc);
        close(fd);
        return 1;
    }

    printf("[findmy-linux] Apple Find My BLE beacon active on hci%d (Interval: %dms)\n",
           hci_id, interval_ms);
    fflush(stdout);

    // Keep daemon running to maintain socket and handle signals
    while (g_running) {
        sleep(1);
    }

    printf("\n[findmy-linux] Shutting down beacon...\n");
    hci_le_set_adv_enable(fd, false);
    close(fd);

    return 0;
}
