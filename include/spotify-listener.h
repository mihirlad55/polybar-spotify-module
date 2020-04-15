#ifndef _SPOTIFY_LISTENER_H_
#define _SPOTIFY_LISTENER_H_

#include <dbus-1.0/dbus/dbus.h>
#include <stdarg.h>

dbus_bool_t send_ipc_polybar(int numOfMsgs, ...);

DBusHandlerResult handle_media_player_signal(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data);

DBusHandlerResult name_owner_changed_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data);

void free_user_data(void *memory);

dbus_bool_t spotify_playing();

dbus_bool_t spotify_paused();

dbus_bool_t spotify_exited();

#endif
