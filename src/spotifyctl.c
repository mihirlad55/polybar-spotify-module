#include "../include/spotifyctl.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

/*************** Constants for DBus ***************/
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

/*** Program Mode ***/
typedef enum {
    MODE_NONE,
    MODE_STATUS,
    MODE_PLAY,
    MODE_PAUSE,
    MODE_PREVIOUS,
    MODE_NEXT,
    MODE_PLAYPAUSE
} ProgMode;

// Predictable errors will be hidden if this is TRUE such as if spotify is not
// running and the status is requested
dbus_bool_t SUPPRESS_ERRORS = 0;

char *get_song_title_from_metadata(DBusMessage *msg) {
    DBusMessageIter iter;

    dbus_message_iter_init(msg, &iter);

    char *title = NULL;

    // The message looks like this:
    // string "org.mpris.MediaPlayer2.Player"
    // array [
    //    dict entry(
    //       string "Metadata"
    //       variant             array [
    //          .
    //          .
    //          .
    //             dict entry(
    //                string "xesam:title"
    //                variant               string "{track title}"
    //             )
    //       ]
    //    )
    // ]
    // The track title is at the path:
    // variant->array[xesam:title]->variant->string

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

    // The message looks like this:
    // string "org.mpris.MediaPlayer2.Player"
    // array [
    //    dict entry(
    //       string "Metadata"
    //       variant             array [
    //          .
    //          .
    //          .
    //             dict entry(
    //                string "xesam:artist"
    //                variant               string "{track artist}"
    //             )
    //       ]
    //    )
    // ]
    // The track title is at the path:
    // variant->array[xesam:artist]->variant->string

    if (iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_ARRAY) &&
        iter_try_step_to_key(&iter, METADATA_ARTIST_KEY) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_VARIANT) &&
        iter_try_step_into_type(&iter, DBUS_TYPE_ARRAY)) {
        artist = iter_get_string(&iter);
    }

    return artist;
}

char *format_output(const char *artist, const char *title,
                    const int max_artist_length, const int max_title_length,
                    const int max_length, const char *format,
                    const char *trunc) {
    // Get total number of each token
    const int NUM_OF_ARTIST_TOK = num_of_matches(format, "%artist%");
    const int NUM_OF_TITLE_TOK = num_of_matches(format, "%title%");

    // Get length difference caused by a single replacement
    const int ARTIST_REPL_DIFF = strlen(artist) - strlen("%artist%");
    const int TITLE_REPL_DIFF = strlen(title) - strlen("%title%");

    // Calculate the total untruncated length of the output
    const int TOTAL_UNTRUNC_LENGTH = strlen(format) +
                                     NUM_OF_ARTIST_TOK * ARTIST_REPL_DIFF +
                                     NUM_OF_TITLE_TOK * TITLE_REPL_DIFF;

    char *output;

    // Truncate artist and title only if total untruncated length > max_length
    // and max_length was specified
    if (max_length == INT_MAX || TOTAL_UNTRUNC_LENGTH > max_length) {
        char *trunc_title;
        char *trunc_artist;

        // Truncate artist and track title using the truncation string
        if (!(trunc_title = str_trunc(title, max_title_length, trunc))) {
            if (!SUPPRESS_ERRORS) {
                fputs(
                    "Failed to truncate title. Please make sure the trunc "
                    "string is smaller than the max title length.\n",
                    stderr);
                exit(1);
            }
        }

        if (!(trunc_artist = str_trunc(artist, max_artist_length, trunc))) {
            if (!SUPPRESS_ERRORS) {
                fputs(
                    "Failed to truncate artist. Please make sure the trunc "
                    "string is smaller than the max artist length.\n",
                    stderr);
            }
            exit(1);
        }

        // Replace all tokens with their values
        char *temp = str_replace_all(format, "%artist%", trunc_artist);
        char *temp2 = str_replace_all(temp, "%title%", trunc_title);

        // Truncate output to max length
        if (!(output = str_trunc(temp2, max_length, trunc))) {
            if (!SUPPRESS_ERRORS) {
                fputs(
                    "Failed to truncate output. Please make sure the trunc "
                    "string is smaller than the max output length.\n",
                    stderr);
            }
            exit(1);
        }

        free(temp);
        free(temp2);
        free(trunc_title);
        free(trunc_artist);
    } else {
        // Replace all tokens with their values
        char *temp = str_replace_all(format, "%artist%", artist);
        output = str_replace_all(temp, "%title%", title);

        free(temp);
    }

    return output;
}

void get_status(DBusConnection *connection, const int max_artist_length,
                const int max_title_length, const int max_length,
                const char *format, const char *trunc) {
    DBusError err;
    dbus_error_init(&err);

    // Send a message requesting the properties
    DBusMessage *msg = dbus_message_new_method_call(
        DESTINATION, PATH, STATUS_IFACE, STATUS_METHOD);

    // Message looks like this:
    // string "org.mpris.MediaPlayer2.Player"
    // string "Metadata"
    dbus_message_append_args(
        msg, DBUS_TYPE_STRING, &STATUS_METHOD_ARG_IFACE_NAME, DBUS_TYPE_STRING,
        &STATUS_METHOD_ARG_PROPERTY_NAME, DBUS_TYPE_INVALID);

    // Send and receive reply
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

    puts(output);

    free(output);
    free(title);
    free(artist);

    dbus_message_unref(reply);
}

void spotify_player_call(DBusConnection *connection, const char *method) {
    DBusError err;
    dbus_error_init(&err);

    // Call a org.mpris.MediaPlayer2.Player method
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
    puts("                              to show. If max-length is specified,");
    puts("                              This will only restrict the length if");
    puts("                              the output length is longer than");
    puts("                              max-length");
    puts("                                Default: No limit");
    puts("    --max-title-length        The maximum length of the track title");
    puts("                              to show. If max-length is specified,");
    puts("                              This will only restrict the length if");
    puts("                              the output length is longer than");
    puts("                              max-length");
    puts("                                Default: No limit");
    puts("    --max-length              The maximum length of the output of");
    puts("                              the status command. This value works");
    puts("                              best as the sum of the max artist and");
    puts("                              max title length if those are");
    puts("                              specified.");
    puts("                              Default: No limit");
    puts("    --format                  The format to display the status in.");
    puts("                              The %artist% and %title% tokens will");
    puts("                              be replaced by the artist name and");
    puts("                                Default: '%artist%: %title%'");
    puts("                              track title, respectively.");
    puts("    --trunc                   The string to use to show that the");
    puts("                              artist name, track title, or output");
    puts("                              was longer than the max length");
    puts("                              specified. This will count towards");
    puts("                              the max lengths. This can be blank.");
    puts("                                Default: '...'");
    puts("    -q                        Hide errors");
    puts("");
    puts("  Examples:");
    puts("    spotifyctl status --format '%artist%: %title%' \\");
    puts("        --max-length 30 --max-artist-length 10 \\");
    puts("        --max-title-length 20 --trunc '...'");
    puts("    If artist name is 'Eminem' and track title is");
    puts("    'Sing For The Moment', the output will be:");
    puts("    Eminem: Sing For The Moment");
    puts("    since the total length is less than 30 characters.");
    puts("");
    puts("    spotifyctl status --format '%artist%: %title%' \\");
    puts("        --max-length 20 --max-artist-length 10 \\");
    puts("        --max-title-length 10 --trunc '...'");
    puts("    If artist name is 'Eminem' and track title is");
    puts("    'Sing For The Moment', the output will be:");
    puts("    Eminem: Sing Fo...");
    puts("    since the total length is less than 30 characters.");
    puts("");
    puts("    spotifyctl status --format '%artist%: %title%' \\");
    puts("        --max-title-length 13 --trunc '...'");
    puts("    If artist name is 'Eminem' and track title is");
    puts("    'Sing For The Moment', the output will be:");
    puts("    Eminem: Sing For T...");
    puts("    since the total length is less than 30 characters.");
}

int main(int argc, char *argv[]) {
    DBusConnection *connection;
    DBusError err;

    // Default options
    ProgMode prog_mode = MODE_NONE;
    int max_artist_length = INT_MAX;
    int max_title_length = INT_MAX;
    int max_length = INT_MAX;
    char *status_format = "%artist%: %title%";
    char *trunc = "...";

    // Parse commandline options
    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            SUPPRESS_ERRORS = 1;
        } else if (strcmp(argv[i], "--max-artist-length") == 0) {
            max_artist_length = atoi(argv[++i]);
            if (max_artist_length <= 0) {
                fputs("Artist length must be a positive integer!\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[i], "--max-title-length") == 0) {
            max_title_length = atoi(argv[++i]);
            if (max_title_length <= 0) {
                fputs("Title length must be a positive integer!\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[i], "--max-length") == 0) {
            max_length = atoi(argv[++i]);
            if (max_length <= 0) {
                fputs("Max length must be a positive integer!\n", stderr);
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

    // Call function based on command supplied
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
