#include "../include/spotify-status.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

const char *DESTINATION = "org.mpris.MediaPlayer2.spotify";
const char *PATH = "/org/mpris/MediaPlayer2";
const char *IFACE = "org.freedesktop.DBus.Properties";
const char *METHOD = "Get";
const char *METHOD_ARG_IFACE_NAME = "org.mpris.MediaPlayer2.Player";
const char *METHOD_ARG_PROPERTY_NAME = "Metadata";

const char *METADATA_TITLE_KEY = "xesam:title";
const char *METADATA_ARTIST_KEY = "xesam:artist";

const int MAX_OUTPUT_LENGTH = 30;
// Leave 3 characters for "..." if cut off and 2 for ": "
const int OUTPUT_PADDING_LENGTH = 5;
// Title + artist length should be less than max output - padding
const int MAX_TITLE_LENGTH = 15;
const int MAX_ARTIST_LENGTH = 10;

char *get_song_title_from_metadata(DBusMessage *msg) {
    DBusMessageIter iter;

    dbus_message_iter_init(msg, &iter);

    char *title = NULL;

    if (iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_ARRAY) &&
        iter_try_step_to_key(&iter, METADATA_TITLE_KEY) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT)) {
        title = iter_get_string(&iter);
    }

    return title;
}

char *get_song_artist_from_metadata(DBusMessage *msg) {
    DBusMessageIter iter;

    dbus_message_iter_init(msg, &iter);

    char *artist = NULL;

    if (iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_ARRAY) &&
        iter_try_step_to_key(&iter, METADATA_ARTIST_KEY) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_ARRAY)) {
        artist = iter_get_string(&iter);
    }

    return artist;
}

char *format_output(char *artist, char *title) {
    size_t title_len = strlen(title);
    size_t artist_len = strlen(artist);

    // +2 = "\n\0"
    const size_t OUTPUT_SIZE = MAX_OUTPUT_LENGTH + 2;

    char *output = (char *)calloc(OUTPUT_SIZE, sizeof(char));
    output[0] = '\0';

    if (title_len + artist_len > MAX_OUTPUT_LENGTH) {
        if (title_len > MAX_TITLE_LENGTH) title_len = MAX_TITLE_LENGTH;
        if (artist_len > MAX_ARTIST_LENGTH) artist_len = MAX_ARTIST_LENGTH;

        strncpy(output, artist, artist_len);
        strcat(output, ": ");
        strncat(output, title, title_len);
        strcat(output, "...\n");
    } else {
        strncpy(output, artist, artist_len);
        strcat(output, ": ");
        strncat(output, title, title_len);
        strcat(output, "\n");
    }

    return output;
}

int main() {
    DBusConnection *connection;
    DBusError err;

    dbus_error_init(&err);

    // Connect to session bus
    if (!(connection = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
        fprintf(stderr, "%s\n", err.message);
        return 1;
    }

    DBusMessage *msg =
        dbus_message_new_method_call(DESTINATION, PATH, IFACE, METHOD);

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &METHOD_ARG_IFACE_NAME,
                             DBUS_TYPE_STRING, &METHOD_ARG_PROPERTY_NAME,
                             DBUS_TYPE_INVALID);

    DBusMessage *reply;
    reply =
        dbus_connection_send_with_reply_and_block(connection, msg, 10000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "%s\n", err.message);
        return 1;
    }

    char *title = get_song_title_from_metadata(reply);
    char *artist = get_song_artist_from_metadata(reply);

    char *output = format_output(artist, title);

    printf("%s", output);

    free(title);
    free(artist);
    free(output);

    dbus_message_unref(reply);
    dbus_connection_unref(connection);

    return 0;
}
