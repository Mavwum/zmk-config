/* Studio owns the LED strip. Keep the ZMK behavior/display API without a second renderer. */
#include "runtime.h"
#include <zmk/rgb_underglow.h>
#include <errno.h>
#if !CONFIG_KEEBART_STUDIO
int ks_lighting_action(unsigned a, int v) { return -ENOTSUP; }
bool ks_lighting_on(void) { return true; }
#endif
int zmk_rgb_underglow_toggle(void) { return ks_lighting_action(0, 0); }
int zmk_rgb_underglow_get_state(bool *s) {
    *s = ks_lighting_on();
    return 0;
}
int zmk_rgb_underglow_on(void) { return ks_lighting_action(1, 1); }
int zmk_rgb_underglow_off(void) { return ks_lighting_action(1, 0); }
int zmk_rgb_underglow_cycle_effect(int d) { return ks_lighting_action(2, d); }
int zmk_rgb_underglow_calc_effect(int d) { return 0; }
int zmk_rgb_underglow_select_effect(int e) { return -ENOTSUP; }
struct zmk_led_hsb zmk_rgb_underglow_calc_hue(int d) { return (struct zmk_led_hsb){0}; }
struct zmk_led_hsb zmk_rgb_underglow_calc_sat(int d) { return (struct zmk_led_hsb){0}; }
struct zmk_led_hsb zmk_rgb_underglow_calc_brt(int d) { return (struct zmk_led_hsb){0}; }
int zmk_rgb_underglow_change_hue(int d) { return ks_lighting_action(3, d * 8); }
int zmk_rgb_underglow_change_sat(int d) { return ks_lighting_action(4, d * 8); }
int zmk_rgb_underglow_change_brt(int d) { return ks_lighting_action(5, d * 2); }
int zmk_rgb_underglow_change_spd(int d) { return ks_lighting_action(6, d * 8); }
int zmk_rgb_underglow_set_hsb(struct zmk_led_hsb c) { return -ENOTSUP; }
