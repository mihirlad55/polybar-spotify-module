#include "../include/utils.h"

#include <stdio.h>
#include <string.h>


void print_string_iter(DBusMessageIter *iter) {
    int type = dbus_message_iter_get_arg_type(iter);

    if (type == DBUS_TYPE_STRING) {
        DBusBasicValue value;
        dbus_message_iter_get_basic(iter, &value);
        printf("%s\n", value.str);
    }
}


dbus_bool_t recurse_iter_of_type(DBusMessageIter *iter, DBusMessageIter
        *subiter, int type) {
    int iter_type = dbus_message_iter_get_arg_type(iter);

    if (iter_type == type) {
        dbus_message_iter_recurse(iter, subiter);
        return TRUE;
    }

    return FALSE;
}


dbus_bool_t recurse_iter_of_signature(DBusMessageIter *iter, DBusMessageIter
        *subiter, char *signature) {
    char *iter_signature = dbus_message_iter_get_signature(iter);
    dbus_bool_t res = FALSE;

    if (strcmp(iter_signature, signature) == 0) {
        dbus_message_iter_recurse(iter, subiter);
        res = TRUE;
    }

    dbus_free(iter_signature);
    return res;
}


dbus_bool_t iter_go_to_key(DBusMessageIter *array_iter, DBusMessageIter
        *entry_iter, char *key) {
    int iter_type = dbus_message_iter_get_arg_type(array_iter);
    char *iter_signature = dbus_message_iter_get_signature(array_iter);

    // Make sure iter is on dict entry and signature matches string-variant
    // entry
    if (iter_type != DBUS_TYPE_DICT_ENTRY ||
            strcmp(iter_signature, "{sv}") != 0) {
        dbus_free(iter_signature);
        return FALSE;
    }

    dbus_free(iter_signature);

    int current_type;
    while ( (current_type = dbus_message_iter_get_arg_type(array_iter)) != DBUS_TYPE_INVALID) {
        recurse_iter_of_type(array_iter, entry_iter, DBUS_TYPE_DICT_ENTRY);

        DBusBasicValue value;
        dbus_message_iter_get_basic(entry_iter, &value);
        char *k = value.str;

        if (strcmp(k, key) == 0) {
            dbus_message_iter_next(entry_iter);
            return TRUE;
        }

        dbus_message_iter_next(array_iter);
    }

    return FALSE;
}
