#include "../include/utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print_string_iter(DBusMessageIter *iter) {
    int type = dbus_message_iter_get_arg_type(iter);

    if (type == DBUS_TYPE_STRING) {
        DBusBasicValue value;
        dbus_message_iter_get_basic(iter, &value);
        printf("%s\n", value.str);
    }
}

char *iter_get_string(DBusMessageIter *iter) {
    int type = dbus_message_iter_get_arg_type(iter);

    char *str;

    if (type == DBUS_TYPE_STRING) {
        DBusBasicValue value;
        dbus_message_iter_get_basic(iter, &value);

        size_t size = strlen(value.str) + 1;
        str = (char *)malloc(size * sizeof(char));
        str[0] = '\0';
        strcpy(str, value.str);

        return str;
    }

    return NULL;
}

dbus_bool_t recurse_iter_of_type(DBusMessageIter *iter,
                                 DBusMessageIter *subiter, int type) {
    int iter_type = dbus_message_iter_get_arg_type(iter);

    if (iter_type == type) {
        dbus_message_iter_recurse(iter, subiter);
        return TRUE;
    }

    return FALSE;
}

dbus_bool_t recurse_iter_of_signature(DBusMessageIter *iter,
                                      DBusMessageIter *subiter,
                                      const char *signature) {
    char *iter_signature = dbus_message_iter_get_signature(iter);
    dbus_bool_t res = FALSE;

    if (strcmp(iter_signature, signature) == 0) {
        dbus_message_iter_recurse(iter, subiter);
        res = TRUE;
    }

    dbus_free(iter_signature);
    return res;
}

dbus_bool_t iter_go_to_key(DBusMessageIter *element_iter,
                           DBusMessageIter *entry_iter, const char *key) {
    int iter_type = dbus_message_iter_get_arg_type(element_iter);
    char *iter_signature = dbus_message_iter_get_signature(element_iter);

    // Make sure iter is on dict entry and signature matches string-variant
    // entry
    if (iter_type != DBUS_TYPE_DICT_ENTRY ||
        strcmp(iter_signature, "{sv}") != 0) {
        dbus_free(iter_signature);
        return FALSE;
    }

    dbus_free(iter_signature);

    int current_type;
    while ((current_type = dbus_message_iter_get_arg_type(element_iter)) !=
           DBUS_TYPE_INVALID) {
        recurse_iter_of_type(element_iter, entry_iter, DBUS_TYPE_DICT_ENTRY);

        DBusBasicValue value;
        dbus_message_iter_get_basic(entry_iter, &value);
        char *k = value.str;

        if (strcmp(k, key) == 0) {
            dbus_message_iter_next(entry_iter);
            return TRUE;
        }

        dbus_message_iter_next(element_iter);
    }

    return FALSE;
}

dbus_bool_t iter_try_step_into_type(DBusMessageIter *iter, int type) {
    DBusMessageIter sub_iter;

    if (!recurse_iter_of_type(iter, &sub_iter, type)) return FALSE;

    *iter = sub_iter;

    return TRUE;
}

dbus_bool_t iter_try_step_to_key(DBusMessageIter *element_iter,
                                 const char *key) {
    DBusMessageIter kv_iter;

    if (!iter_go_to_key(element_iter, &kv_iter, key)) return FALSE;

    *element_iter = kv_iter;

    return TRUE;
}

dbus_bool_t iter_try_step_into_signature(DBusMessageIter *iter,
                                         const char *signature) {
    DBusMessageIter sub_iter;

    if (!recurse_iter_of_signature(iter, &sub_iter, signature)) return FALSE;

    *iter = sub_iter;

    return TRUE;
}

dbus_bool_t msleep(long milliseconds) {
    struct timespec ts;
    int res;

    if (milliseconds < 0) {
        return FALSE;
    }

    time_t sec = milliseconds / 1000;
    long ns = (milliseconds % 1000) * 1000 * 1000;

    ts.tv_sec = sec;
    ts.tv_nsec = ns;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return TRUE;
}

char *join_path(const char *p1, const char *p2) {
    size_t len1 = strlen(p1) + 1;  // Including null character
    size_t len2 = strlen(p2) + 1;  // Including null character

    char *res;

    if (p1[len1 - 2] != '/') {
        res = (char *)malloc((len1 + len2 + 1) * sizeof(char));
        res[0] = '\0';
        strcat(res, p1);
        strcat(res, "/");
        strcat(res, p2);
        return res;
    } else if (p1[len1 - 2] == '/') {
        res = (char *)malloc((len1 + len2) * sizeof(char));
        res[0] = '\0';
        strcat(res, p2);
        return res;
    } else {
        return NULL;
    }
}

dbus_bool_t get_polybar_ipc_paths(const char *ipc_path, char **ptr_paths[],
                                  size_t *num_of_paths) {
    DIR *d;
    struct dirent *dir;
    size_t i = 0;

    // Start with 3 paths allocated
    *num_of_paths = 3;
    char **paths = (char **)malloc(*num_of_paths * sizeof(char *));

    d = opendir(ipc_path);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            const char *name = dir->d_name;

            if (strncmp(name, "polybar_mqueue", 14) == 0) {
                char *path = join_path(ipc_path, name);
                size_t len = strlen(path) + 1;  // +1 for null char

                if (i >= *num_of_paths) {
                    // Reallocate 2 additional paths
                    paths = (char **)realloc(paths, (i + 3) * sizeof(char *));
                }

                paths[i] = path;
                i++;
            }
        }

        // Get actual number of paths
        *num_of_paths = i;
        // Reallocate array for actual number of paths
        paths = (char **)realloc(paths, *num_of_paths * sizeof(char *));
        // Assign address of array to pointer to array
        *ptr_paths = paths;
    } else {
        closedir(d);
        return FALSE;
    }

    closedir(d);
    return TRUE;
}

