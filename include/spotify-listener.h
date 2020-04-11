#ifndef _SPOTIFY_LISTENER_H_
#define _SPOTIFY_LISTENER_H_

dbus_bool_t send_ipc_polybar(const char *message);

DBusHandlerResult handle_media_player_signal(DBusConnection *connection,
        DBusMessage *message, void *user_data);

DBusHandlerResult name_owner_changed_handler (DBusConnection *connection,
        DBusMessage *message, void *user_data);

void free_user_data(void *memory);

dbus_bool_t spotify_playing();

dbus_bool_t spotify_paused();

dbus_bool_t spotify_exited();

#endif
