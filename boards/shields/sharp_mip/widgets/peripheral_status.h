/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

/* Pre-decoded frames and playback settings consumed by the status widget. */
struct sharp_mip_animation_config {
    const lv_img_dsc_t *frames;
    size_t frame_count;
    size_t idle_frame;
    uint32_t frame_interval_ms;
};

extern const struct sharp_mip_animation_config sharp_mip_animation;

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
    struct status_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
