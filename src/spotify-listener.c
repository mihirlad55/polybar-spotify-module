#include <stdio.h>
#include <inttypes.h>
#include <dbus-1.0/dbus/dbus.h>

#include <string.h>
#include <unistd.h>

#include <stdlib.h>

#include "../include/utils.h"

#include "../include/spotify-listener.h"


const char *POLYBAR_IPC_DIRECTORY = "/tmp";

dbus_bool_t send_ipc_polybar(char *message) {
    FILE *fp;

    fp = fopen("/tmp/polybar_mqueue.3905", "w");

    fprintf(fp, "%s\n", message);

    fclose(fp);
    
}

DBusHandlerResult handle_media_player_signal(DBusConnection *connection,
        DBusMessage *message, void *user_data) {
    DBusMessageIter iter;

    dbus_bool_t is_spotify = FALSE;

    dbus_message_iter_init(message, &iter);
    int current_type;

    // Check if first string is org.mpris.MediaPlayer2.Player
    if ( (current_type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID ) {
        if (current_type == DBUS_TYPE_STRING) {
            DBusBasicValue value;
            dbus_message_iter_get_basic(&iter, &value);

            if (strcmp(value.str, "org.mpris.MediaPlayer2.Player") != 0) {
                printf("%s", "First string not org.mpris.MediaPlayer2.Player");
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
        } else { 
            printf("%s", "First element not string");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    } else {
        printf("%s", "No data in signal");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (!dbus_message_iter_next(&iter)) {
        printf("%s", "Second element does not exist");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }


    DBusMessageIter sub_iter;
    DBusMessageIter entry_iter;
    DBusMessageIter variant_iter;
    DBusMessageIter array_iter;
    DBusMessageIter value_iter;
    DBusMessageIter final_iter;
    
    // Recurse into array
    if (!recurse_iter_of_type(&iter, &sub_iter, DBUS_TYPE_ARRAY)) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    // Go to value with Metadata key
    if (!iter_go_to_key(&sub_iter, &entry_iter, "Metadata")) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    // Recurse into variant value
    if (!recurse_iter_of_type(&entry_iter, &variant_iter, DBUS_TYPE_VARIANT)) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    // Recurse into array of metadata
    if (!recurse_iter_of_signature(&variant_iter, &array_iter, "a{sv}")) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    // Go to value with key mpris:trackid
    if (!iter_go_to_key(&array_iter, &value_iter, "mpris:trackid")) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    if (!recurse_iter_of_type(&value_iter, &final_iter, DBUS_TYPE_VARIANT)) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    current_type = dbus_message_iter_get_arg_type(&final_iter);
    if (!current_type == DBUS_TYPE_STRING) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusBasicValue value;
    dbus_message_iter_get_basic(&final_iter, &value);

    if (strncmp(value.str, "spotify", 7) == 0) {
        printf("IT IS SPOTIFY\n");
        is_spotify = TRUE;
    }

    if (is_spotify) {
        // Recurse into array
        if (!recurse_iter_of_type(&iter, &sub_iter, DBUS_TYPE_ARRAY)) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        DBusMessageIter status_iter;
        if (!iter_go_to_key(&sub_iter, &entry_iter, "PlaybackStatus")) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        // Recurse into variant value
        if (!recurse_iter_of_type(&entry_iter, &status_iter, DBUS_TYPE_VARIANT)) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        current_type = dbus_message_iter_get_arg_type(&status_iter);
        if (!current_type == DBUS_TYPE_STRING) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        dbus_message_iter_get_basic(&status_iter, &value);

        if (strcmp(value.str, "Paused") == 0) {
            printf("Song paused\n");
            send_ipc_polybar("hook:module/playpause3");
        } else if (strcmp(value.str, "Playing") == 0) {
            printf("Song is playing\n");
            send_ipc_polybar("hook:module/playpause2");
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
    printf("Running handler\n");
}


DBusHandlerResult name_owner_changed_handler (DBusConnection *connection,
        DBusMessage *message, void *user_data) {
    printf("Starting handler for name owner changed\n");

    const char *name;
    const char *old_owner;
    const char *new_owner;

   if (!dbus_message_get_args(message, NULL,
            DBUS_TYPE_STRING, &name,
            DBUS_TYPE_STRING, &old_owner,
            DBUS_TYPE_STRING, &new_owner,
            DBUS_TYPE_INVALID)) {
        printf("NOT HANDLED\n");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (strcmp(name, "org.mpris.MediaPlayer2.spotify") == 0 &&
            strcmp(new_owner, "") == 0) {
        printf("Spotify disconnected\n");
        // Hide pause button
        send_ipc_polybar("hook:module/playpause1");
        // Hide track info
        send_ipc_polybar("hook:module/spotify1"); 
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


void free_user_data(void *memory) {

}


const char *PROPERTIES_CHANGED_MATCH = "interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path='/org/mpris/MediaPlayer2'";
const char *NAME_OWNER_CHANGED_MATCH = "interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/freedesktop/DBus'";

int main() {
    DBusConnection *connection;
    DBusError err;

    dbus_error_init(&err);

    
    // Connect to session bus
    if ( !(connection = dbus_bus_get(DBUS_BUS_SESSION, &err)) ) {
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
    if(!dbus_connection_add_filter(connection, name_owner_changed_handler,
                NULL, free_user_data)) {
        fprintf(stderr, "%s", "Failed to add NameOwnerChanged handler\n");
        return 1;
    }


    // Read messages and call handlers when neccessary
    while (dbus_connection_read_write_dispatch(connection, -1)) {
        printf("In dispatch loop\n");
    }


    dbus_connection_unref(connection);
    return 0;
}
