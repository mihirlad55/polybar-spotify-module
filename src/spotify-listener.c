#include "../include/spotify-listener.h"

#include <dbus-1.0/dbus/dbus.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/utils.h"

#ifdef VERBOSE
const dbus_bool_t VERBOSE = TRUE;
#else
const dbus_bool_t VERBOSE = FALSE;
#endif

const char *POLYBAR_IPC_DIRECTORY = "/tmp";

char *last_trackid = NULL;

typedef enum { PLAYING, PAUSED, EXITED } spotify_state;

spotify_state CURRENT_SPOTIFY_STATE = EXITED;

dbus_bool_t update_last_trackid(const char *trackid) {
    size_t size = strlen(trackid) + 1;

    last_trackid = (char *)realloc(last_trackid, size);
    last_trackid[0] = '\0';

    strcpy(last_trackid, trackid);

    if (last_trackid != NULL)
        return TRUE;
    else
        return FALSE;
}

dbus_bool_t spotify_update_track(const char *current_trackid) {
    if (last_trackid != NULL && strcmp(current_trackid, last_trackid) != 0) {
        printf("%s", "Track changed\n");
        // Send message to update track name
        if (send_ipc_polybar(1, "hook:module/spotify2")) return TRUE;
    }
    return FALSE;
}

dbus_bool_t spotify_playing() {
    if (CURRENT_SPOTIFY_STATE != PLAYING) {
        printf("Song is playing\n");
        // Show pause, next, and previous button
        if (send_ipc_polybar(4, "hook:module/playpause2",
                             "hook:module/previous2", "hook:module/next2",
                             "hook:module/spotify2")) {
            CURRENT_SPOTIFY_STATE = PLAYING;
            return TRUE;
        }
    }
    return FALSE;
}

dbus_bool_t spotify_paused() {
    if (CURRENT_SPOTIFY_STATE != PAUSED) {
        printf("Song paused\n");
        // Show play, next, and previous button
        if (send_ipc_polybar(4, "hook:module/playpause3",
                             "hook:module/previous2", "hook:module/next2",
                             "hook:module/spotify2")) {
            CURRENT_SPOTIFY_STATE = PAUSED;
            return TRUE;
        }
    }
    return FALSE;
}

dbus_bool_t spotify_exited() {
    if (CURRENT_SPOTIFY_STATE != EXITED) {
        // Hide all buttons and track display
        if (send_ipc_polybar(4, "hook:module/playpause1",
                             "hook:module/previous1", "hook:module/next1",
                             "hook:module/spotify1")) {
            CURRENT_SPOTIFY_STATE = EXITED;
            return TRUE;
        }
    }
    return FALSE;
}

dbus_bool_t send_ipc_polybar(int numOfMsgs, ...) {
    char **paths;
    size_t num_of_paths;
    va_list args;

    // Pass address of pointer to array of strings
    get_polybar_ipc_paths(POLYBAR_IPC_DIRECTORY, &paths, &num_of_paths);

    for (size_t p = 0; p < num_of_paths; p++) {
        FILE *fp;

        va_start(args, numOfMsgs);
        for (int m = 0; m < numOfMsgs; m++) {
            const char *message = va_arg(args, char *);

            fp = fopen(paths[p], "w");
            fprintf(fp, "%s\n", message);
            printf("%s%s%s%s%s\n", "Sending the message '", message, "' to '",
                   paths[p], "'");

            fclose(fp);

            // Without sleep, requests are sometimes ignored
            msleep(50);
        }
        va_end(args);

        free(paths[p]);
    }

    free(paths);

    return TRUE;
}

DBusHandlerResult handle_media_player_signal(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data) {
    DBusMessageIter iter;

    dbus_bool_t is_spotify = FALSE;

    dbus_message_iter_init(message, &iter);
    int current_type;

    // Check if first string is org.mpris.MediaPlayer2.Player
    if ((current_type = dbus_message_iter_get_arg_type(&iter)) !=
        DBUS_TYPE_INVALID) {
        if (current_type == DBUS_TYPE_STRING) {
            DBusBasicValue value;
            dbus_message_iter_get_basic(&iter, &value);

            if (strcmp(value.str, "org.mpris.MediaPlayer2.Player") != 0) {
                if (VERBOSE)
                    printf("%s",
                           "First string not org.mpris.MediaPlayer2.Player");
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
        } else {
            if (VERBOSE) printf("%s", "First element not string");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    } else {
        if (VERBOSE) printf("%s", "No data in signal");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (!dbus_message_iter_next(&iter)) {
        if (VERBOSE) printf("%s", "Second element does not exist");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    DBusMessageIter sub_iter;

    // Recurse into array
    if (!(recurse_iter_of_type(&iter, &sub_iter, DBUS_TYPE_ARRAY) &&
          // Go to value with Metadata key
          iter_try_step_to_key(&sub_iter, "Metadata") &&
          // Step into variant value
          iter_try_step_into_type(&sub_iter, DBUS_TYPE_VARIANT) &&
          // Step into array of metadata
          iter_try_step_into_signature(&sub_iter, "a{sv}") &&
          // Go to value with key mpris:trackid
          iter_try_step_to_key(&sub_iter, "mpris:trackid") &&
          // Step into container
          iter_try_step_into_type(&sub_iter, DBUS_TYPE_VARIANT) &&
          // Verify string type
          dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_STRING)) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    DBusBasicValue value;

    dbus_message_iter_get_basic(&sub_iter, &value);
    const char *trackid = value.str;

    if (strncmp(trackid, "spotify", 7) == 0) {
        if (VERBOSE) printf("Spotify Detected\n");
        spotify_update_track(trackid);
        update_last_trackid(trackid);
        is_spotify = TRUE;
    }

    if (is_spotify) {
        // Recurse into array
        if (!(recurse_iter_of_type(&iter, &sub_iter, DBUS_TYPE_ARRAY) &&
              // Step to PlaybackStatus key
              iter_try_step_to_key(&sub_iter, "PlaybackStatus") &&
              // Recurse into variant value
              iter_try_step_into_type(&sub_iter, DBUS_TYPE_VARIANT) &&
              // Verify string type
              dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_STRING)) {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        dbus_message_iter_get_basic(&sub_iter, &value);
        const char *status = value.str;

        if (strcmp(status, "Paused") == 0) {
            spotify_paused();
        } else if (strcmp(status, "Playing") == 0) {
            spotify_playing();
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
    printf("Running handler\n");
}

DBusHandlerResult name_owner_changed_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data) {
    if (VERBOSE) printf("Starting handler for name owner changed\n");

    const char *name;
    const char *old_owner;
    const char *new_owner;

    if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &name,
                               DBUS_TYPE_STRING, &old_owner, DBUS_TYPE_STRING,
                               &new_owner, DBUS_TYPE_INVALID)) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (strcmp(name, "org.mpris.MediaPlayer2.spotify") == 0 &&
        strcmp(new_owner, "") == 0) {
        printf("Spotify disconnected\n");
        spotify_exited();
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void free_user_data(void *memory) {}

const char *PROPERTIES_CHANGED_MATCH =
    "interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',"
    "path='/org/mpris/MediaPlayer2'";
const char *NAME_OWNER_CHANGED_MATCH =
    "interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/"
    "freedesktop/DBus'";

int main() {
    DBusConnection *connection;
    DBusError err;

    dbus_error_init(&err);

    // Connect to session bus
    if (!(connection = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
        fprintf(stderr, "%s\n", err.message);
        return 1;
    }

    // Receive messages for PropertiesChanged signal to detect track changes
    // or spotify launching
    dbus_bus_add_match(connection, PROPERTIES_CHANGED_MATCH, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "%s\n", err.message);
        return 1;
    }

    // Receive messages for NameOwnerChanged signal to detect spotify exiting
    dbus_bus_add_match(connection, NAME_OWNER_CHANGED_MATCH, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "%s\n", err.message);
        return 1;
    }

    // Register handler for PropertiesChanged signal
    if (!dbus_connection_add_filter(connection, handle_media_player_signal,
                                    NULL, free_user_data)) {
        fprintf(stderr, "%s", "Failed to add properties changed handler\n");
        return 1;
    }

    // Register handler for NameOwnerChanged signal
    if (!dbus_connection_add_filter(connection, name_owner_changed_handler,
                                    NULL, free_user_data)) {
        fprintf(stderr, "%s", "Failed to add NameOwnerChanged handler\n");
        return 1;
    }

    // Read messages and call handlers when neccessary
    while (dbus_connection_read_write_dispatch(connection, -1)) {
        if (VERBOSE) printf("In dispatch loop\n");
    }

    dbus_connection_unref(connection);
    return 0;
}
