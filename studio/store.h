#pragma once
#include "core.h"
#define KS_STORE_CHUNK 736
#define KS_STORE_CHUNKS ((sizeof(struct ks_config) + KS_STORE_CHUNK - 1) / KS_STORE_CHUNK)
_Static_assert(KS_STORE_CHUNKS <= 32, "bounded storage bank records");
struct ks_store {
    struct ks_config banks[2];
    uint32_t heads[2][2];
    unsigned received[2];
    uint16_t lengths[2][KS_STORE_CHUNKS];
    int selected;
};
typedef int (*ks_write_record)(const char *, const void *, size_t);
void ks_store_init(struct ks_store *);
bool ks_store_receive(struct ks_store *, const char *, const void *, size_t);
bool ks_store_load(struct ks_store *, struct ks_state *);
int ks_store_save(struct ks_store *, const struct ks_config *, ks_write_record);
