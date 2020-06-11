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

// Used to check if track has changed
char *last_trackid = NULL;

// Current state of spotify
typedef enum { PLAYING, PAUSED, EXITED } SpotifyState;
SpotifyState CURRENT_SPOTIFY_STATE = EXITED;

// DBus signals to listen for
const char *PROPERTIES_CHANGED_MATCH =
    "interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',"
    "path='/org/mpris/MediaPlayer2'";
const char *NAME_OWNER_CHANGED_MATCH =
    "interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/"
    "freedesktop/DBus'";


dbus_bool_t update_last_trackid(const char *trackid) {
    // +1 for null char
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
    // If trackid didn't change
    if (last_trackid != NULL && strcmp(current_trackid, last_trackid) != 0) {
        puts("Track Changed");
        // Send message to update track name
        if (send_ipc_polybar(1, "hook:module/spotify2")) return TRUE;
    }
    return FALSE;
}

dbus_bool_t spotify_playing() {
    if (CURRENT_SPOTIFY_STATE != PLAYING) {
        puts("Song is playing");
        // Show pause, next, and previous button on polybar
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
        puts("Song is paused");
        // Show play, next, and previous button on polybar
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
        // Hide all buttons and track display on polybar
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
            fputs(message, fp);
            printf("%s%s%s%s%s\n", "Sending the message '", message, "' to '",
                   paths[p], "'");

            fclose(fp);

            // Without sleep, requests are sometimes ignored
            msleep(10);
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
    if (VERBOSE) puts("Running handle_media_player_signal");
    DBusMessageIter iter;
    DBusMessageIter sub_iter;
    dbus_bool_t is_spotify = FALSE;
    dbus_message_iter_init(message, &iter);

    /**
     * Format of PropertiesChanged signal
     * string "org.mpris.MediaPlayer2.Player"
     * array [
     *     dict entry(
     *         string "Metadata"
     *         variant  array [
     *             dict entry(
     *                 string "mpris:trackid"
     *                 variant  string "spotify:track:xxxxxxxxxxxxx"
     *             )
     *           .
     *           .
     *           .
     *         ]
     *     )
     *     dict entry(
     *         string "PlaybackStatus"
     *         variant  string "Paused"
     *     )
     * ]
     *
     */

    char *interface_name = iter_get_string(&iter);

    // Check if interface is correct
    if (interface_name != NULL &&
        strcmp(interface_name, "org.mpris.MediaPlayer2.Player") != 0) {
        if (VERBOSE)
            puts(
                "Interface of PropertiesChanged signal not "
                "org.mpris.MediaPlayer2.Player");
        free(interface_name);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    free(interface_name);

    dbus_message_iter_next(&iter);

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

    // Make sure trackid begins with spotify
    char *trackid = iter_get_string(&sub_iter);
    if (trackid != NULL && strncmp(trackid, "spotify", 7) == 0) {
        spotify_update_track(trackid);
        update_last_trackid(trackid);
        is_spotify = TRUE;

        if (VERBOSE) puts("Spotify Detected");
    }

    free(trackid);

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

        // Update polybar modules
        char *status = iter_get_string(&sub_iter);
        if (strcmp(status, "Paused") == 0) {
            spotify_paused();
        } else if (strcmp(status, "Playing") == 0) {
            spotify_playing();
        }

        free(status);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult name_owner_changed_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data) {
    if (VERBOSE) puts("Starting handler for name owner changed");

    const char *name;
    const char *old_owner;
    const char *new_owner;

    /**
     * Format of NameOwnerChanged signal
     * string "name"
     * string "old owner"
     * string "new owner"
     *
     */

    // Try to get message arguments
    if (!dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &name,
                               DBUS_TYPE_STRING, &old_owner, DBUS_TYPE_STRING,
                               &new_owner, DBUS_TYPE_INVALID)) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    // If name matches spotify and new owner is "", spotify disconnected
    if (strcmp(name, "org.mpris.MediaPlayer2.spotify") == 0 &&
        strcmp(new_owner, "") == 0) {
        puts("Spotify disconnected");
        spotify_exited();
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void free_user_data(void *memory) {}

int main() {
    DBusConnection *connection;
    DBusError err;

    dbus_error_init(&err);

    // Connect to session bus
    if (!(connection = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
        fputs(err.message, stderr);
        return 1;
    }

    // Receive messages for PropertiesChanged signal to detect track changes
    // or spotify launching
    dbus_bus_add_match(connection, PROPERTIES_CHANGED_MATCH, &err);
    if (dbus_error_is_set(&err)) {
        fputs(err.message, stderr);
        return 1;
    }

    // Receive messages for NameOwnerChanged signal to detect spotify exiting
    dbus_bus_add_match(connection, NAME_OWNER_CHANGED_MATCH, &err);
    if (dbus_error_is_set(&err)) {
        fputs(err.message, stderr);
        return 1;
    }

    // Register handler for PropertiesChanged signal
    if (!dbus_connection_add_filter(connection, handle_media_player_signal,
                                    NULL, free_user_data)) {
        fputs("Failed to add properties changed handler", stderr);
        return 1;
    }

    // Register handler for NameOwnerChanged signal
    if (!dbus_connection_add_filter(connection, name_owner_changed_handler,
                                    NULL, free_user_data)) {
        fputs("Failed to add NameOwnerChanged handler", stderr);
        return 1;
    }

    // Read messages and call handlers when neccessary
    while (dbus_connection_read_write_dispatch(connection, -1)) {
        if (VERBOSE) puts("In dispatch loop");
    }

    dbus_connection_unref(connection);
    return 0;
}
