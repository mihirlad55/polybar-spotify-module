#ifndef _SPOTIFY_STATUS_H_
#define _SPOTIFY_STATUS_H_

#include <dbus-1.0/dbus/dbus.h>

char *get_song_title_from_metadata(DBusMessage *msg);

char *get_song_artist_from_metadata(DBusMessage *msg);

char *format_output(char *artist, char *title);

void get_status(DBusConnection *connection);

void spotify_player_call(DBusConnection *connection, const char *method);

void print_usage();

int main(int argc, char *argv[]);

#endif
