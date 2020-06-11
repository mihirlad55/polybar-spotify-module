#ifndef _UTILS_H_
#define _UTILS_H_

#include <dbus-1.0/dbus/dbus.h>

char *iter_get_string(DBusMessageIter *iter);

void print_string_iter(DBusMessageIter *iter);

dbus_bool_t recurse_iter_of_type(DBusMessageIter *iter,
                                 DBusMessageIter *subiter, int type);

dbus_bool_t recurse_iter_of_signature(DBusMessageIter *iter,
                                      DBusMessageIter *subiter,
                                      const char *signature);

dbus_bool_t iter_go_to_key(DBusMessageIter *array_iter,
                           DBusMessageIter *entry_iter, const char *key);

dbus_bool_t iter_try_step_into_type(DBusMessageIter *iter, int type);

dbus_bool_t iter_try_step_to_key(DBusMessageIter *element_iter,
                                 const char *key);

dbus_bool_t iter_try_step_into_signature(DBusMessageIter *iter,
                                         const char *signature);

dbus_bool_t msleep(long milliseconds);

dbus_bool_t get_polybar_ipc_paths(const char *ipc_path, char **ptr_paths[],
                                  size_t *num_of_paths);

char *join_path(const char *p1, const char *p2);

char* str_replace_all(char *str, char *find, char *repl);

char* str_trunc(char *str, const int max_len, char *trunc);

int num_of_matches(char *str, char *find);

#endif
