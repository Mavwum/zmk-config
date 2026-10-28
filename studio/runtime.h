#pragma once
#include "core.h"
bool ks_frame(uint8_t[KS_KEYS][3]);
int ks_lighting_action(unsigned action, int value);
bool ks_lighting_on(void);

void ks_peer_release(uint8_t out[12]);
