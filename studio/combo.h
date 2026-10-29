#pragma once
#include "core.h"
struct ks_combo_key {uint16_t code;uint8_t state;};
struct ks_combo_engine {uint32_t cookie;struct ks_combo_key keys[KS_KEYS];};
typedef void (*ks_emit)(void *, unsigned position, uint16_t code, bool down, int64_t at);
void ks_combo_input(struct ks_combo_engine *, const struct ks_config *, unsigned position,
                    uint16_t code, bool down, int64_t at, ks_emit, void *);
void ks_combo_tick(struct ks_combo_engine *, const struct ks_config *, int64_t now, ks_emit,
                   void *);
int64_t ks_combo_deadline(const struct ks_combo_engine *, const struct ks_config *);
