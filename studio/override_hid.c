/* Studio-only QMK override adapter. ZMK event/HID integration follows its MIT-licensed
 * hid_listener.c; the override engine retains its original QMK GPL header. */
#include "core.h"
#include "consumer_keys.h"
#include "override_bridge.h"
#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
extern struct k_mutex state_mutex;
extern const struct ks_override *ks_override_entries(void);
static uint8_t source_layer, normal_weak, override_weak, suppressed;
static bool initialized, consumer_dirty;
static unsigned bypass;
static uint32_t clock_ms(void){return (uint32_t)k_uptime_get();}
static void wait_time(unsigned n){k_msleep(n);}
static uint32_t usage(uint8_t code){return ks_key_usage(code);}
static void press(uint8_t code){zmk_hid_press(usage(code));if(ks_consumer_usage(code))consumer_dirty=true;}
static void release(uint8_t code){zmk_hid_release(usage(code));if(ks_consumer_usage(code))consumer_dirty=true;}
static void update_mods(void){zmk_hid_masked_modifiers_set(suppressed);zmk_hid_implicit_modifiers_press((normal_weak & ~suppressed)|override_weak);}
static void send(void){update_mods();zmk_endpoints_send_report(HID_USAGE_KEY);if(consumer_dirty){consumer_dirty=false;zmk_endpoints_send_report(HID_USAGE_CONSUMER);}}
static void weak(uint8_t mods){override_weak=mods;update_mods();}
static void mask(uint8_t mods){suppressed=mods;update_mods();}
static void shortcut(uint8_t m){normal_weak|=m;update_mods();}
static const struct ks_override_io io={.mods=zmk_hid_get_explicit_mods,.press=press,.release=release,.send=send,.weak=weak,.mask=mask,.shortcut=shortcut,.clock=clock_ms,.wait=wait_time};
static void init(void){if(!initialized){initialized=true;ks_override_init(ks_override_entries(),&io);}}
static void tick(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(override_work,tick);
static void tick(struct k_work *work){
    k_mutex_lock(&state_mutex,K_FOREVER);ks_override_tick();if(ks_override_pending())k_work_reschedule(&override_work,K_MSEC(1));k_mutex_unlock(&state_mutex);
}
void ks_override_lifecycle_cancel(void){if(initialized){ks_override_cancel();k_work_cancel_delayable(&override_work);}}
// Observe the originating layer of resolved hold/tap, sticky and physical actions.
// Wrapping only this Studio build avoids changing upstream ZMK behavior sources.
extern int __real_zmk_behavior_invoke_binding(const struct zmk_behavior_binding *,struct zmk_behavior_binding_event,bool);
int __wrap_zmk_behavior_invoke_binding(const struct zmk_behavior_binding *binding,struct zmk_behavior_binding_event event,bool down){
    k_mutex_lock(&state_mutex,K_FOREVER);
    uint8_t previous=source_layer;source_layer=event.layer<16?event.layer:0;
    // QMK macro bytecode injects reports directly, outside process_key_override.
    bool macro=event.position>=KS_KEYS+KS_ENCODERS+KS_COMBOS+512 && event.position<KS_KEYS+KS_ENCODERS+KS_COMBOS+768;
    if(macro)bypass++;
    int result=__real_zmk_behavior_invoke_binding(binding,event,down);
    if(macro)bypass--;
    source_layer=previous;k_mutex_unlock(&state_mutex);return result;
}
static uint16_t qmk_code(const struct zmk_keycode_state_changed *e){
    uint16_t c;
    if(e->usage_page==HID_USAGE_CONSUMER){
        c=ks_consumer_code(e->keycode);
        if(c==0xffff)return c;
    } else {
        if(e->usage_page!=HID_USAGE_KEY || e->keycode>255)return 0xffff;
        c=e->keycode;
    }
    uint8_t mods=e->implicit_modifiers;
    if(mods && !(mods&0xf0))c|=(uint16_t)mods<<8;
    else if(mods && !(mods&15))c|=0x1000|((uint16_t)mods<<4);
    return c;
}
static int key_event(const zmk_event_t *event){
    const struct zmk_keycode_state_changed *e=as_zmk_keycode_state_changed(event);
    if(!e)return ZMK_EV_EVENT_BUBBLE;
    k_mutex_lock(&state_mutex,K_FOREVER);init();
    uint16_t code=qmk_code(e);
    bool pass=true;
    if(!bypass && code!=0xffff){pass=ks_override_process(code,source_layer,e->state);k_work_reschedule(&override_work,K_MSEC(1));}
    if(pass){
        if(e->usage_page==HID_USAGE_CONSUMER)consumer_dirty=true;
        if(e->state){
            if(!is_mod(e->usage_page,e->keycode) && zmk_hid_is_pressed(ZMK_HID_USAGE(e->usage_page,e->keycode))){zmk_hid_release(ZMK_HID_USAGE(e->usage_page,e->keycode));send();}
            zmk_hid_press(ZMK_HID_USAGE(e->usage_page,e->keycode));zmk_hid_register_mods(e->explicit_modifiers);normal_weak=e->implicit_modifiers;
        }else{
            zmk_hid_release(ZMK_HID_USAGE(e->usage_page,e->keycode));
#if IS_ENABLED(CONFIG_ZMK_HID_SEPARATE_MOD_RELEASE_REPORT)
            send();
#endif
            zmk_hid_unregister_mods(e->explicit_modifiers);normal_weak=0;
        }
        send();
    }
    k_mutex_unlock(&state_mutex);return ZMK_EV_EVENT_BUBBLE;
}
void ks_override_report(uint16_t code,bool down){
    k_mutex_lock(&state_mutex,K_FOREVER);init();ks_override_process(code,source_layer,down);k_mutex_unlock(&state_mutex);
}
ZMK_LISTENER(keebart_override_hid,key_event);
ZMK_SUBSCRIPTION(keebart_override_hid,zmk_keycode_state_changed);
