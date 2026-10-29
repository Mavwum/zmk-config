#include "device_name.h"
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
BUILD_ASSERT(CONFIG_ZMK_USB_INIT_PRIORITY > 93, "Load the device name before USB enumeration");
BUILD_ASSERT(CONFIG_ZMK_BLE_INIT_PRIORITY < 93, "Mount the guarded Bluetooth settings backend first");
static struct kn_state name_state;
static uint8_t descriptor[242], served_descriptor[242];
static struct k_spinlock descriptor_lock;
static char text[KN_MAX+1];
static void apply_name(void) {
    uint8_t next[242];size_t size=kn_usb_descriptor(name_state.record,next);
    k_spinlock_key_t key=k_spin_lock(&descriptor_lock);memcpy(descriptor,next,size);k_spin_unlock(&descriptor_lock,key);
    memcpy(text,name_state.record+9,name_state.record[8]);text[name_state.record[8]]=0;
    (void)bt_set_name(text);
    struct bt_data ad[]={
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,BT_BYTES_LIST_LE16(CONFIG_BT_DEVICE_APPEARANCE)),
        BT_DATA_BYTES(BT_DATA_FLAGS,(BT_LE_AD_GENERAL|BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_SOME,BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
    };
    // Update an existing undirected advertisement without restarting the radio,
    // disconnecting hosts, or changing identities/bonds. Directed/connected
    // states use the new GAP name and pick it up at their next advertisement.
    (void)bt_le_adv_update_data(ad,ARRAY_SIZE(ad),NULL,0);
}
static int load(const char *key,size_t len,settings_read_cb read,void *arg) {
    uint8_t record[KN_RECORD];
    if(!key || strcmp(key,"record") || len!=sizeof(record) || read(arg,record,len)!=(ssize_t)len || !kn_valid(record,len))return -EINVAL;
    memcpy(name_state.record,record,len);return 0;
}
static int commit(void) { apply_name();return 0; }
SETTINGS_STATIC_HANDLER_DEFINE(keebart_name,"keebart_name",NULL,load,commit,NULL);
static int init_name(void) {
    kn_init(&name_state,CONFIG_USB_DEVICE_PRODUCT,NULL);
    // BLE has mounted the guarded settings backend by this priority. Load ONLY
    // this namespace before USB enumeration; never replay pairing callbacks.
    int rc=settings_load_subtree("keebart_name");
    if(rc){kn_init(&name_state,CONFIG_USB_DEVICE_PRODUCT,NULL);}
    apply_name();return 0;
}
SYS_INIT(init_name,APPLICATION,93);
const uint8_t *keebart_usb_name(void) {
    k_spinlock_key_t key=k_spin_lock(&descriptor_lock);memcpy(served_descriptor,descriptor,descriptor[0]);k_spin_unlock(&descriptor_lock,key);
    return served_descriptor[0]?served_descriptor:NULL;
}
struct name_readback { const uint8_t *expected; bool matched; };
static int verify_saved(const char *key,size_t len,settings_read_cb read,void *arg,void *context) {
    struct name_readback *check=context;uint8_t record[KN_RECORD];
    if(key && !strcmp(key,"record") && len==KN_RECORD && read(arg,record,len)==(ssize_t)len)
        check->matched=kn_valid(record,len) && !memcmp(record,check->expected,len);
    return 0;
}
static bool persist_name(const uint8_t record[KN_RECORD]) {
    if(settings_save_one("keebart_name/record",record,KN_RECORD))return false;
    struct name_readback check={record,false};
    return !settings_load_subtree_direct("keebart_name",verify_saved,&check) && check.matched;
}
bool ksn_reset(void) {
    // The authenticated reset already restored configuration. Persist only this
    // name record; Bluetooth identities and bonds are in separate namespaces.
    if(!kn_reset(&name_state,CONFIG_USB_DEVICE_PRODUCT,persist_name))return false;
    apply_name();return true;
}
void ksn_task(void) {
    bool pending=name_state.pending;kn_task(&name_state,persist_name);
    if(pending && name_state.result==KN_OK)apply_name();
}
void ksn_handle(const uint8_t input[32],uint8_t output[32],bool authorized) {
    memset(output,0,32);memcpy(output,input,5);
    if(memcmp(input,"KS\x03\x10",4) || input[5] || input[6]){output[5]=KN_INVALID;return;}
    output[5]=kn_handle(&name_state,input+7,output+6,authorized);
}
