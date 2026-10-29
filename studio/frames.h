#pragma once
#include "hardware.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
struct ks_frames {
    bool committed, has_display;
    uint16_t sequence;
    uint32_t received;
    int64_t at, committed_at;
    uint8_t staged[KS_HALF_LEDS][3], active[KS_HALF_LEDS][3];
};
int ks_frame_receive(struct ks_frames *, const uint8_t *, size_t, int64_t);

#define KS_FRAME_PACKET_SIZE (4 + KS_HALF_LEDS * 3)
size_t ks_frame_packet(const uint8_t colors[KS_HALF_LEDS][3], uint16_t sequence, unsigned start,
                       unsigned mtu, uint8_t packet[KS_FRAME_PACKET_SIZE]);

void ks_frames_init(struct ks_frames *);
void ks_frames_disconnect(struct ks_frames *);
void ks_frames_output(const struct ks_frames *, bool awake, int64_t now,
                      uint8_t out[KS_HALF_LEDS][3]);
