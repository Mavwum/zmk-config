#include "frames.h"
#include "lighting_defaults.h"
#include <string.h>
int ks_frame_receive(struct ks_frames *s, const uint8_t *p, size_t len, int64_t now) {
    if (len < 7 || len > KS_FRAME_PACKET_SIZE)
        return -1;
    uint16_t seq = p[0] | ((uint16_t)p[1] << 8);
    unsigned start = p[2], n = p[3];
    if (!n || n > KS_HALF_LEDS || start + n > KS_HALF_LEDS || len != 4 + n * 3)
        return -1;
    for (unsigned i = 4; i < len; i++)
        if (p[i] > 30)
            return -1;
    if (seq != s->sequence) {
        if (s->received && now - s->at < 1500 && (int16_t)(seq - s->sequence) <= 0)
            return -1;
        s->sequence = seq;
        s->received = 0;
    }
    memcpy(s->staged[start], p + 4, n * 3);
    for (unsigned i = start; i < start + n; i++)
        s->received |= 1U << i;
    s->at = now;
    if (s->received == (1U << KS_HALF_LEDS) - 1) {
        memcpy(s->active, s->staged, sizeof(s->active));
        s->committed_at = now;
        s->committed = s->has_display = true;
        return 1;
    }
    return 0;
}

size_t ks_frame_packet(const uint8_t colors[KS_HALF_LEDS][3], uint16_t sequence, unsigned start,
                       unsigned mtu, uint8_t packet[KS_FRAME_PACKET_SIZE]) {
    if (start >= KS_HALF_LEDS || mtu < 10)
        return 0;
    // ATT write header: 3 bytes; our frame header: 4 bytes.
    unsigned count = (mtu - 7) / 3;
    if (count > KS_HALF_LEDS - start)
        count = KS_HALF_LEDS - start;
    packet[0] = sequence;
    packet[1] = sequence >> 8;
    packet[2] = start;
    packet[3] = count;
    memcpy(packet + 4, colors[start], count * 3);
    return 4 + count * 3;
}

void ks_frames_init(struct ks_frames *s) {
    memset(s, 0, sizeof(*s));
    s->has_display = true;
    for (unsigned i = 0; i < KS_HALF_LEDS; i++) {
        s->active[i][0] = KS_DEFAULT_RED * KS_DEFAULT_BRIGHTNESS / 255;
        s->active[i][1] = KS_DEFAULT_GREEN * KS_DEFAULT_BRIGHTNESS / 255;
        s->active[i][2] = KS_DEFAULT_BLUE * KS_DEFAULT_BRIGHTNESS / 255;
    }
}
void ks_frames_disconnect(struct ks_frames *s) {
    // Drop partial packets and sequence history, preserving the last complete
    // display (including an explicit off frame). Nothing is written to flash.
    s->sequence = 0;
    s->received = 0;
    s->committed = false;
    s->at = s->committed_at = 0;
}
void ks_frames_output(const struct ks_frames *s, bool awake, int64_t now,
                      uint8_t out[KS_HALF_LEDS][3]) {
    // Fresh central frames carry its global activity state. Without a stream,
    // the peripheral uses its own activity so standalone lighting still sleeps.
    if (awake || (s->committed && now - s->committed_at < 1500))
        memcpy(out, s->active, sizeof(s->active));
    else
        memset(out, 0, sizeof(s->active));
}
