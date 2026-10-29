#pragma once
#define QK_MODS 0x0100
#define QK_MODS_MAX 0x1fff
#define QK_LCTL 0x100
#define QK_LSFT 0x200
#define QK_LALT 0x400
#define QK_LGUI 0x800
#define QK_RCTL 0x1100
#define QK_RSFT 0x1200
#define QK_RALT 0x1400
#define QK_RGUI 0x1800
#define QK_KEY_OVERRIDE_TOGGLE 0x7c5d
#define QK_KEY_OVERRIDE_ON 0x7c5e
#define QK_KEY_OVERRIDE_OFF 0x7c5f
#define SAFE_RANGE 0x7e40
#define KC_NO 0
#define IS_BASIC_KEYCODE(c) ((c)>=4 && (c)<=0xa4)
#define IS_MODIFIER_KEYCODE(c) ((c)>=0xe0 && (c)<=0xe7)
#define MOD_BIT(c) (1U<<((c)&7))
