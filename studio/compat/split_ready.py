"""Studio-only readiness bridge for the pinned ZMK split transport.

Lighting must not start its own ATT requests before native discovery and CCC
acknowledgements finish. Callbacks publish atomics only; no new Bluetooth work.
"""
import pathlib,sys
source=pathlib.Path(sys.argv[1]).read_text()
def replace(old,new):
 global source
 if source.count(old)!=1: raise SystemExit('Pinned ZMK split source changed: '+old[:70])
 source=source.replace(old,new,1)
replace('struct peripheral_slot {\n', '''struct peripheral_slot {
    atomic_t keebart_ready;
''')
needle='int release_peripheral_slot(int index) {'
bridge='''// Private Studio readiness, separate from merely connected/encrypted.
// Native discovery can enqueue several CCC operations from Bluetooth RX.
// Keep the lighting client silent until all of those operations acknowledge.
#define KS_SPLIT_DISCOVERED BIT(0)
#define KS_SPLIT_POSITIONS BIT(1)
#define KS_SPLIT_SENSORS BIT(2)
static void keebart_split_subscribed(struct bt_conn *conn, uint8_t err,
                                    struct bt_gatt_subscribe_params *params) {
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    if (!slot) return;
    unsigned bit;
    if (params == &slot->subscribe_params) bit = 1;
#if ZMK_KEYMAP_HAS_SENSORS
    else if (params == &slot->sensor_subscribe_params) bit = 2;
#endif
    else return;
    if (err) atomic_clear_bit(&slot->keebart_ready, bit);
    else atomic_set_bit(&slot->keebart_ready, bit);
}
bool keebart_split_ready(struct bt_conn *conn) {
    struct peripheral_slot *slot = peripheral_slot_for_conn(conn);
    unsigned required = KS_SPLIT_DISCOVERED | KS_SPLIT_POSITIONS;
#if ZMK_KEYMAP_HAS_SENSORS
    required |= KS_SPLIT_SENSORS;
#endif
    return slot && (atomic_get(&slot->keebart_ready) & required) == required;
}

'''
replace(needle,bridge+needle)
replace('    LOG_DBG("Releasing peripheral slot at %d", index);','''    atomic_clear(&slot->keebart_ready);
    LOG_DBG("Releasing peripheral slot at %d", index);''')
replace('''static int split_central_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params) {
''','''static int split_central_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params) {
    params->subscribe = keebart_split_subscribed;
''')
replace('    int err = bt_gatt_subscribe(conn, params);', '    int err = bt_gatt_subscribe(conn, params);\n    if (err == -EALREADY && params->ccc_handle &&\n        !atomic_test_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING))\n        keebart_split_subscribed(conn, 0, params);')
# Zephyr does not deliver a final NULL callback after BT_GATT_ITER_STOP.
# Mark the successful early-stop path, not exhaustion/error with missing handles.
replace('    return subscribed ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;',
        '    if (subscribed) atomic_set_bit(&slot->keebart_ready, 0);\n'
        '    return subscribed ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;')
pathlib.Path(sys.argv[2]).write_text(source)
