#ifndef config_H
#define config_H

#include <stdint.h>
#include <stdbool.h>

#include "payloads/s5l8747x.h"
#include "payloads/s5l8947x.h"

typedef struct {
    uint16_t cpid;
    const char *name;
    int large_leak;
    int overwrite_size;
    uint32_t overwrite_addr;
    bool needs_overwrite_suffix;
    const void *payload;
    size_t payload_len;
} exploit_config_t;

const exploit_config_t configs[] = {
    {
        .cpid = 0x8747,
        .name = "s5l8747x",
        .large_leak = 41,
        .overwrite_size = 0x774,
        .overwrite_addr = 0x22000300,
        .needs_overwrite_suffix = false,
        .payload = payload_s5l8747x,
        .payload_len = sizeof(payload_s5l8747x)
    },
    {
        .cpid = 0x8947,
        .name = "s5l8947x",
        .large_leak = 626,
        .overwrite_size = 0x674,
        .overwrite_addr = 0x34000000,
        .needs_overwrite_suffix = true,
        .payload = payload_s5l8947x,
        .payload_len = sizeof(payload_s5l8947x)
    }
};

static const exploit_config_t *get_config(uint16_t cpid) {
    for (int i = 0; i < sizeof(configs) / sizeof(exploit_config_t); i++) {
        const exploit_config_t *config = &configs[i];
        if (config->cpid == cpid) {
            return config;
        }
    }

    return NULL;
}

#endif
