#ifndef _SPOTIFY_STATUS_H_
#define _SPOTIFY_STATUS_H_

#include <dbus-1.0/dbus/dbus.h>

/**
 * Extract the title of the currently playing song on spotify from a
 * Metadata property DBusMessage
 *
 * @param DBusMessage* msg The Metadata property DBusMessage
 * @returns char* The title of the currently playing track
 */
char *get_song_title_from_metadata(DBusMessage *msg);

/**
 * Extract the artist of the currently playing song on spotify from a
 * Metadata property DBusMessage
 *
 * @param DBusMessage* msg The Metadata property DBusMessage
 * @returns char* The artist of the currently playing track
 */
char *get_song_artist_from_metadata(DBusMessage *msg);

/**
 * Build the output message according to the specified format options
 *
 * @param char* artist The artist name
 * @param char* title The track title
 * @param int max_artist_length The maximum length of the artist in the output
 * @param int max_title_length The maximum length of the title in the output
 * @param int max_length The maximum length of the output string
 * @param char* format The format string specifying the output. This can contain
 *                     the %artist% and %title% tokens which will be replaced by
 *                     the artist and title specified in the arguments.
 * @param char* trunc The string to use to indicate that the artist, title, or
 *                    output was truncated. This will be how the artist, title
 *                    or output ends and will honor the max length constraints.
 *
 * @returns char* The format string with %artist% replaced by the song artist,
 *                %title% replaced by the song title. If max_length is INT_MAX,
 *                artist will be truncated if it is longer than
 *                max_artist_length, and title will be truncated if it is longer
 *                than max_title_length. If max_length is not INT_MAX, the
 *                artist and title will only be truncated if the entire output
 *                string is shorter than max_length, otherwise it will be
 *                truncated as normal. In truncating a string, the end of the
 *                string will be replaced with trunc while sataisfying the
 *                max length constraints.
 */
char *format_output(const char *artist, const char *title,
                    const int max_artist_length, const int max_title_length,
                    const int max_length, const char *format,
                    const char *trunc);

/**
 * Prints the status output message according to the specified format options
 * after making a method call to spotify to obtain the artist and title
 *
 * @param DBusConnection connection The DBusConnection object
 * @param int max_artist_length The maximum length of the artist in the output
 * @param int max_title_length The maximum length of the title in the output
 * @param int max_length The maximum length of the output string
 * @param char* format The format string specifying the output. This can contain
 *                     the %artist% and %title% tokens which will be replaced by
 *                     the artist and title specified in the arguments.
 * @param char* trunc The string to use to indicate that the artist, title, or
 *                    output was truncated. This will be how the artist, title
 *                    or output ends and will honor the max length constraints.
 *
 */
void get_status(DBusConnection *connection, const int max_artist_length,
                const int max_title_length, const int max_length,
                const char *format, const char *trunc);

/**
 * Call the specified org.mpris.MediaPlayer2.Player method
 *
 * @param DBusConnection* connection The DBusConnection object
 * @param const char* method The name of the org.mpris.MediaPlayer2.Player
 *                           method to call.
 */
void spotify_player_call(DBusConnection *connection, const char *method);

/**
 * Print spotifyctl usage information
 */
void print_usage();

/**
 * The main entrypoint
 *
 * @param int argc The number of arguments
 * @param char** argv An array of arguments
 */
int main(int argc, char *argv[]);

#endif  // _SPOTIFY_STATUS_H_ include guard
