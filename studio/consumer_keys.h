#pragma once
#include <stdint.h>

/* QMK consumer keycodes 0xa8..0xc2, mapped to HID Consumer Page usages.
 * Kept identical to KEYCODE2CONSUMER by the shared HID regression test. */
static inline uint16_t ks_consumer_usage(uint16_t code) {
    static const uint16_t usages[] = {
        0x00e2, 0x00e9, 0x00ea, 0x00b5, 0x00b6, 0x00b7, 0x00cd,
        0x0183, 0x00cc, 0x018a, 0x0192, 0x0194, 0x0221, 0x0223,
        0x0224, 0x0225, 0x0226, 0x0227, 0x022a, 0x00b3, 0x00b4,
        0x006f, 0x0070, 0x019f, 0x01cb, 0x029f, 0x02a0,
    };
    return code >= 0xa8 && code <= 0xc2 ? usages[code - 0xa8] : 0;
}
static inline uint16_t ks_consumer_code(uint16_t usage) {
    for (uint16_t code = 0xa8; code <= 0xc2; code++)
        if (ks_consumer_usage(code) == usage) return code;
    return 0xffff;
}
static inline uint32_t ks_key_usage(uint16_t code) {
    uint16_t consumer = ks_consumer_usage(code);
    return consumer ? 0x000c0000u | consumer : 0x00070000u | code;
}
