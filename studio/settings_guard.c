/* Studio only: a failed settings mount must never run commit callbacks.
 * bt_enable() sets its enabled flag BEFORE mounting settings. On mount failure
 * its controller/threads are not started. ZMK main ignores settings init errors
 * and otherwise commits "bt", which can wait forever for that controller.
 * Keep USB health reporting alive and all persistent data untouched instead.
 */
#include <zephyr/settings/settings.h>
extern int keebart_storage_error(void);
extern int __real_settings_load(void);
extern int __real_settings_load_subtree(const char *subtree);
extern int __real_settings_load_subtree_direct(const char *subtree,
                                             settings_load_direct_cb cb, void *param);
extern int __real_settings_save_one(const char *name, const void *value, size_t len);

int __wrap_settings_load(void) {
    int error = keebart_storage_error();
    return error ? error : __real_settings_load();
}
int __wrap_settings_load_subtree(const char *subtree) {
    int error = keebart_storage_error();
    return error ? error : __real_settings_load_subtree(subtree);
}
int __wrap_settings_load_subtree_direct(const char *subtree,
                                      settings_load_direct_cb cb, void *param) {
    int error = keebart_storage_error();
    return error ? error : __real_settings_load_subtree_direct(subtree, cb, param);
}
int __wrap_settings_save_one(const char *name, const void *value, size_t len) {
    int error = keebart_storage_error();
    return error ? error : __real_settings_save_one(name, value, len);
}
