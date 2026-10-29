#pragma once
#include "core.h"
struct ks_hit {
    uint8_t position;
    int64_t at;
};
struct ks_renderer {
    struct ks_hit hits[32];
    unsigned next;
    uint8_t heat[KS_KEYS];
    int64_t decay;
};
void ks_render_press(struct ks_renderer *, unsigned position, int64_t now);
void ks_render(struct ks_renderer *, const struct ks_config *, unsigned layer, uint8_t locks,
               bool awake, int64_t now, uint8_t out[KS_KEYS][3]);
extern const uint8_t ks_led_positions[KS_KEYS];
