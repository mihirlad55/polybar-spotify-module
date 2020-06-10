#include "../include/spotifyctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

const char *DESTINATION = "org.mpris.MediaPlayer2.spotify";
const char *PATH = "/org/mpris/MediaPlayer2";

const char *STATUS_IFACE = "org.freedesktop.DBus.Properties";
const char *STATUS_METHOD = "Get";
const char *STATUS_METHOD_ARG_IFACE_NAME = "org.mpris.MediaPlayer2.Player";
const char *STATUS_METHOD_ARG_PROPERTY_NAME = "Metadata";

const char *PLAYER_IFACE = "org.mpris.MediaPlayer2.Player";
const char *PLAYER_METHOD_PLAY = "Play";
const char *PLAYER_METHOD_PAUSE = "Pause";
const char *PLAYER_METHOD_PLAYPAUSE = "PlayPause";
const char *PLAYER_METHOD_NEXT = "Next";
const char *PLAYER_METHOD_PREVIOUS = "Previous";

const char *METADATA_TITLE_KEY = "xesam:title";
const char *METADATA_ARTIST_KEY = "xesam:artist";

typedef enum {
    MODE_NONE,
    MODE_STATUS,
    MODE_PLAY,
    MODE_PAUSE,
    MODE_PREVIOUS,
    MODE_NEXT,
    MODE_PLAYPAUSE
} ProgMode;

dbus_bool_t SUPPRESS_ERRORS = 0;

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

char *format_output(char *artist, char *title, int max_artist_length,
                    int max_title_length, int max_length, char *format,
                    char *trunc) {
    // Truncate artist and track title using the truncation string
    if (!(title = str_trunc(title, max_title_length, trunc))) {
        if (!SUPPRESS_ERRORS) {
            fputs(
                "Failed to truncate title. Please make sure the trunc string "
                "is smaller "
                "than the max title length.\n",
                stderr);
            exit(1);
        }
    }
    if (!(artist = str_trunc(artist, max_artist_length, trunc))) {
        if (!SUPPRESS_ERRORS) {
            fputs(
                "Failed to truncate artist. Please make sure the trunc string "
                "is smaller "
                "than the max title length.\n",
                stderr);
        }
        exit(1);
    }

    // Replace all tokens with their values
    char *temp = str_replace_all(format, "%artist%", artist);
    char *output = str_replace_all(temp, "%title%", title);

    // Truncate output to max length
    output = str_trunc(output, max_length, "");

    // Allocate extra character to add newline
    const size_t OUTPUT_SIZE = (strlen(output) + 1) * sizeof(char);
    output = (char *)realloc(output, OUTPUT_SIZE);

    strcat(output, "\n");

    free(temp);
    free(title);
    free(artist);

    return output;
}

void get_status(DBusConnection *connection, int max_artist_length,
                int max_title_length, int max_length, char *format,
                char *trunc) {
    DBusError err;
    dbus_error_init(&err);

    DBusMessage *msg = dbus_message_new_method_call(
        DESTINATION, PATH, STATUS_IFACE, STATUS_METHOD);

    dbus_message_append_args(
        msg, DBUS_TYPE_STRING, &STATUS_METHOD_ARG_IFACE_NAME, DBUS_TYPE_STRING,
        &STATUS_METHOD_ARG_PROPERTY_NAME, DBUS_TYPE_INVALID);

    DBusMessage *reply;
    reply =
        dbus_connection_send_with_reply_and_block(connection, msg, 10000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        if (!SUPPRESS_ERRORS) fputs(err.message, stderr);
        exit(1);
    }

    char *title = get_song_title_from_metadata(reply);
    char *artist = get_song_artist_from_metadata(reply);

    char *output = format_output(artist, title, max_artist_length,
                                 max_title_length, max_length, format, trunc);

    printf("%s", output);

    free(output);

    dbus_message_unref(reply);
}

void spotify_player_call(DBusConnection *connection, const char *method) {
    DBusError err;
    dbus_error_init(&err);

    DBusMessage *msg =
        dbus_message_new_method_call(DESTINATION, PATH, PLAYER_IFACE, method);

    dbus_connection_send_with_reply_and_block(connection, msg, 10000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        if (!SUPPRESS_ERRORS) fputs(err.message, stderr);
        exit(1);
    }
}

void print_usage() {
    puts("usage: spotifyctl [ -q ] [options] <command>");
    puts("");
    puts("  Commands:");
    puts("    play           Play spotify");
    puts("    pause          Pause spotify");
    puts("    playpause      Toggle the play/pause state on spotify");
    puts("    next           Go to the next track on spotify");
    puts("    previous       Go to the previous track on spotify");
    puts("    status         Print the status of spotify including the track");
    puts("                   title and artist name.");
    puts("");
    puts("  Options:");
    puts("    --max-artist-length       The maximum length of the artist name");
    puts("                              to show");
    puts("                                Default: 10");
    puts("    --max-title-length        The maximum length of the track title");
    puts("                              to show");
    puts("                                Default: 15");
    puts("    --max-length              The maximum length of the output of");
    puts("                              the status command");
    puts("    --max-length                Default: 30");
    puts("    --format                  The format to display the status in.");
    puts("                              The %artist% and %title% tokens will");
    puts("                              be replaced by the artist name and");
    puts("                                Default: '%artist%: %title%'");
    puts("                              track title, respectively.");
    puts("    --trunc                   The string to use to show that the");
    puts("                              artist name or track title were");
    puts("                              longer than the max length specified.");
    puts("                              This will count towards the max");
    puts("                              lengths. This can be blank.");
    puts("                                Default: '...'");
    puts("    -q                        Hide errors");
    puts("");
    puts("  Examples:");
    puts("    spotifyctl status --format '%artist%: %title%' \\");
    puts("        --max-length 30 --max-artist-length 10 \\");
    puts("        --max-title-length 20 --trunc '...'");
    puts("    If artist name is 'Eminem' and track title is");
    puts("    'Sing for the Moment', the output will be:");
    puts("    Eminem: Sing for the...");
}

int main(int argc, char *argv[]) {
    DBusConnection *connection;
    DBusError err;
    ProgMode prog_mode = MODE_NONE;

    int max_artist_length = 10;
    int max_title_length = 15;
    int max_length = 30;
    char *status_format = "%artist%: %title%";
    char *trunc = "...";

    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            SUPPRESS_ERRORS = 1;
        } else if (strcmp(argv[i], "--max-artist-length") == 0) {
            max_artist_length = atoi(argv[++i]);
            if (max_artist_length == 0) {
                fputs("Artist length must be a positive integer!", stderr);
                print_usage();
                return 1;
            }
        } else if (strcmp(argv[i], "--max-title-length") == 0) {
            max_title_length = atoi(argv[++i]);
            if (max_title_length == 0) {
                fputs("Title length must be a positive integer!", stderr);
                print_usage();
                return 1;
            }
        } else if (strcmp(argv[i], "--max-length") == 0) {
            max_length = atoi(argv[++i]);
            if (max_length == 0) {
                fputs("Max length must be a positive integer!", stderr);
                print_usage();
                return 1;
            }
        } else if (strcmp(argv[i], "--format") == 0) {
            status_format = argv[++i];
        } else if (strcmp(argv[i], "--trunc") == 0) {
            trunc = argv[++i];
        } else if (strcmp(argv[i], "status") == 0) {
            prog_mode = MODE_STATUS;
        } else if (strcmp(argv[i], "play") == 0) {
            prog_mode = MODE_PLAY;
        } else if (strcmp(argv[i], "pause") == 0) {
            prog_mode = MODE_PAUSE;
        } else if (strcmp(argv[i], "playpause") == 0) {
            prog_mode = MODE_PLAYPAUSE;
        } else if (strcmp(argv[i], "next") == 0) {
            prog_mode = MODE_NEXT;
        } else if (strcmp(argv[i], "previous") == 0) {
            prog_mode = MODE_PREVIOUS;
        } else if (strcmp(argv[i], "help") == 0) {
            print_usage();
            return 0;
        } else {
            fprintf(stderr, "Invalid option '%s'\n", argv[i]);
            fputs("usage: spotifyctl [ -q ] [options] <command>\n", stderr);
            fputs("Try 'spotifyctl help' for more information\n", stderr);
            return 1;
        }
    }

    dbus_error_init(&err);

    // Connect to session bus
    if (!(connection = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
        if (!SUPPRESS_ERRORS) fputs(err.message, stderr);
        return 1;
    }

    switch (prog_mode) {
        case MODE_NONE:
            fputs("No command specified\n", stderr);
            fputs("Try 'spotifyctl help' for more information\n", stderr);
            dbus_connection_unref(connection);
            return 1;

        case MODE_STATUS:
            get_status(connection, max_artist_length, max_title_length,
                       max_length, status_format, trunc);
            break;

        case MODE_PLAY:
            spotify_player_call(connection, PLAYER_METHOD_PLAY);
            break;

        case MODE_PAUSE:
            spotify_player_call(connection, PLAYER_METHOD_PAUSE);
            break;

        case MODE_PLAYPAUSE:
            spotify_player_call(connection, PLAYER_METHOD_PLAYPAUSE);
            break;

        case MODE_NEXT:
            spotify_player_call(connection, PLAYER_METHOD_NEXT);
            break;

        case MODE_PREVIOUS:
            spotify_player_call(connection, PLAYER_METHOD_PREVIOUS);
            break;
    }

    dbus_connection_unref(connection);

    return 0;
}
