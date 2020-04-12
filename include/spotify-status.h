#ifndef _SPOTIFY_STATUS_H_
#define _SPOTIFY_STATUS_H_

#include <dbus-1.0/dbus/dbus.h>

char *get_song_title_from_metadata(DBusMessage *msg);

char *get_song_artist_from_metadata(DBusMessage *msg);

char *format_output(char *artist, char *title);

int main();

#endif
