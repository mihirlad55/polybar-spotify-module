#ifndef _UTILS_H_
#define _UTILS_H_

#include <dbus-1.0/dbus/dbus.h>

dbus_bool_t iter_get_string(DBusMessageIter *iter, char *str);

void print_string_iter(DBusMessageIter *iter);

dbus_bool_t recurse_iter_of_type(DBusMessageIter *iter, DBusMessageIter
        *subiter, int type);

dbus_bool_t recurse_iter_of_signature(DBusMessageIter *iter, DBusMessageIter
        *subiter, char *signature);

dbus_bool_t iter_go_to_key(DBusMessageIter *array_iter, DBusMessageIter
        *entry_iter, char *key);

dbus_bool_t msleep(long milliseconds);

#endif
