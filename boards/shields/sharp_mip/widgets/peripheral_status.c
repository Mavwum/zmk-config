/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_status.h"

/* Idle time before the current cycle is allowed to finish. */
#define ANIMATION_IDLE_TIMEOUT_MS 2000

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
/* LVGL image object that displays the current frame. */
static lv_obj_t *animation_image;
/* Timer that waits for the idle timeout after the last key press. */
static lv_timer_t *idle_timer;
/* Timer that calls advance_animation() at each frame interval. */
static lv_timer_t *animation_timer;
/* True while the animation timer is advancing frames. */
static bool animation_active;
/* When true, stop after the animation returns to frame 0. */
static bool stop_after_cycle;
/* Zero-based index of the displayed frame. */
static size_t displayed_frame;

/* State passed through the ZMK listener; true means a key is pressed. */
struct typing_activity_state {
    bool pressed;
};

/* Converts the ZMK event (eh) into a simple pressed state. */
static struct typing_activity_state typing_activity_get_state(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct typing_activity_state){0};
    }

    /* Points to the key-position data carried by the ZMK event. */
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    return (struct typing_activity_state){.pressed = event != NULL && event->state};
}

/* state contains the detected key state. On a key press, start the animation if needed,
 * reset the idle countdown, and cancel a pending stop if typing resumes before cycle end. */
static void typing_activity_update_cb(struct typing_activity_state state) {
    if (!state.pressed) {
        return;
    }

    stop_after_cycle = false;
    if (!animation_active) {
        animation_active = true;
        lv_timer_resume(animation_timer);
    }
    lv_timer_reset(idle_timer);
    lv_timer_resume(idle_timer);
}

/* Registers the callback for keyboard events; ZMK generates typing_activity_init(). */
ZMK_DISPLAY_WIDGET_LISTENER(typing_activity, struct typing_activity_state,
                            typing_activity_update_cb, typing_activity_get_state)
/* Notify this listener when a key position state changes. */
ZMK_SUBSCRIPTION(typing_activity, zmk_position_state_changed);

/* Called by the expired idle timer; schedule stopping at the end of the cycle. */
static void request_stop_after_cycle(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    stop_after_cycle = animation_active;
    lv_timer_pause(idle_timer);
}

/* Display the configured idle frame and pause both timers. */
static void set_idle_frame(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    animation_active = false;
    stop_after_cycle = false;
    displayed_frame = sharp_mip_animation.idle_frame;
    lv_img_set_src(animation_image, &sharp_mip_animation.frames[displayed_frame]);
    lv_timer_pause(idle_timer);
    lv_timer_pause(animation_timer);
}

/* Advance to the next frame and check whether a stop was requested. */
static void advance_animation(lv_timer_t *timer) {
    ARG_UNUSED(timer);

    displayed_frame = (displayed_frame + 1) % sharp_mip_animation.frame_count;
    lv_img_set_src(animation_image, &sharp_mip_animation.frames[displayed_frame]);

    if (stop_after_cycle && displayed_frame == sharp_mip_animation.idle_frame) {
        set_idle_frame(timer);
    }
}

struct peripheral_status_state {
    bool connected;
};

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw battery
    draw_battery(canvas, state);

    // Draw output status
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

#ifdef CONFIG_SHARP_MIP_ROTATE_180 // sets positions for default and flipped canvases
int art_pos = 20;
int top_pos = 0;
#else
int art_pos = 0;
int top_pos = 92;
#endif

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, top_pos, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_set_style_bg_color(widget->obj, LVGL_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);
    /* Create the image object and initially display the standing frame at index 0. */
    animation_image = lv_img_create(widget->obj);
    displayed_frame = sharp_mip_animation.idle_frame;
    lv_img_set_src(animation_image, &sharp_mip_animation.frames[displayed_frame]);
    lv_obj_align(animation_image, LV_ALIGN_TOP_LEFT, art_pos, 0);
    /* Create both timers paused; keyboard input resumes them when needed. */
    idle_timer = lv_timer_create(request_stop_after_cycle, ANIMATION_IDLE_TIMEOUT_MS, NULL);
    lv_timer_pause(idle_timer);
    animation_timer = lv_timer_create(advance_animation, sharp_mip_animation.frame_interval_ms, NULL);
    lv_timer_pause(animation_timer);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();
    typing_activity_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
