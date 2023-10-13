#include "../include/spotify-listener.h"

#include <dbus-1.0/dbus/dbus.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../include/utils.h"

#ifdef VERBOSE
const dbus_bool_t VERBOSE = TRUE;
#else
const dbus_bool_t VERBOSE = FALSE;
#endif

// const char *POLYBAR_IPC_DIRECTORY = "/tmp";

// Used to check if track has changed
char *last_trackid = NULL;

// Used to identify spotify for Play/Pause
char *dbus_senderid = NULL;

dbus_bool_t is_spotify = FALSE;

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

dbus_bool_t get_spotify_status() {
  GDBusProxy *proxy;
	GDBusConnection *conn;
	GError *error = NULL;
	const char *info;
	GVariant *variant;
  dbus_bool_t player_is_spotify = FALSE;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,
				      G_DBUS_PROXY_FLAGS_NONE,
				      NULL,				/* GDBusInterfaceInfo */
				      "org.mpris.MediaPlayer2.playerctld",		/* name */
				      "/org/mpris/MediaPlayer2",	/* object path */
				      "org.mpris.MediaPlayer2",	/* interface */
				      NULL,				/* GCancellable */
				      &error);
	g_assert_no_error(error);

	/* read the player property of the interface */
  variant = g_dbus_proxy_get_cached_property(proxy, "Identity");

  g_assert(variant != NULL);
  if(g_variant_is_of_type(variant, G_VARIANT_TYPE_STRING) == TRUE){
    g_variant_get(variant, "s", &info);
  }

	g_variant_unref(variant);
	printf("Current mpris media player is: %s\n", info);

	g_object_unref(proxy);
	g_object_unref(conn);

  player_is_spotify = strcmp(info, "Spotify") == 0;
  return player_is_spotify;
}

const char* get_now_playing() {
  GDBusProxy *proxy;
	GDBusConnection *conn;
	GError *error = NULL;
	const char *trackid;
  const char *playback_status;
	GVariant *variant;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,
				      G_DBUS_PROXY_FLAGS_NONE,
				      NULL,				/* GDBusInterfaceInfo */
				      "org.mpris.MediaPlayer2.spotify",		/* name */
				      "/org/mpris/MediaPlayer2",	/* object path */
				      "org.mpris.MediaPlayer2.Player",	/* interface */
				      NULL,				/* GCancellable */
				      &error);
	g_assert_no_error(error);

  // Format of the metadata...
  // (<{'mpris:trackid': <'/com/spotify/track/0pJqL2QSmybnABaFFTJtCs'>,
  //  'mpris:length': <uint64 125996000>,
  //  'mpris:artUrl': <'https://i.scdn.co/image/...'>,
  //  'xesam:album': <'Astral Distance'>,
  //  'xesam:albumArtist': <['Muni Yogi']>,
  //  'xesam:artist': <['Muni Yogi']>,
  //  'xesam:autoRating': <0.56000000000000005>,
  //  'xesam:discNumber': <1>,
  //  'xesam:title': <'Astral Distance'>,
  //  'xesam:trackNumber': <1>,
  //  'xesam:url': <'https://open.spotify.com/track/0pJqL2QSmybnABaFFTJtCs'>}>,)

	variant = g_dbus_proxy_get_cached_property(proxy, "Metadata");
	g_assert(variant != NULL);

  g_variant_lookup(variant, "mpris:trackid", "s", &trackid);
  printf("Current track is: %s\n", trackid);

	g_variant_unref(variant);
	/* read the playback status of the interface */
	variant = g_dbus_proxy_get_cached_property(proxy, "PlaybackStatus");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &playback_status);

	g_variant_unref(variant);

	printf("Spotify playback is: %s\n", playback_status);

  if(strcmp(playback_status, "Playing") == 0){
    spotify_playing();
  }
  else{
    spotify_paused();
  }

	g_object_unref(proxy);
	g_object_unref(conn);

  return trackid;
}

dbus_bool_t update_last_trackid(const char *trackid) {
  if (trackid != NULL) {
    // +1 for null char
    size_t size = strlen(trackid) + 1;

    last_trackid = (char *)realloc(last_trackid, size);
    last_trackid[0] = '\0';

    strcpy(last_trackid, trackid);

    return TRUE;
  } else {
    return FALSE;
  }
}

dbus_bool_t spotify_update_track(const char *current_trackid) {
  // If trackid didn't change
  if (last_trackid != NULL && strcmp(current_trackid, last_trackid) != 0) {
    puts("Track Changed");
    // Send message to update track name
    if (polybar_msg(1, "#spotify.hook.1")) return TRUE;
  }
  return FALSE;
}

dbus_bool_t spotify_update_sender(const char *senderid) {
  if (senderid != NULL) {
    // +1 for null char
    size_t size = strlen(senderid) + 1;

    dbus_senderid = (char *)realloc(dbus_senderid, size);
    dbus_senderid[0] = '\0';

    strcpy(dbus_senderid, senderid);

    return TRUE;
  } else {
    return FALSE;
  }
}

dbus_bool_t spotify_playing() {
  if (CURRENT_SPOTIFY_STATE != PLAYING) {
    puts("Song is playing");
    // Show pause, next, and previous button on polybar
    if (polybar_msg(4, "#playpause.hook.1",
          "#previous.hook.1", "#next.hook.1",
          "#spotify.hook.1")) {
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
    if (polybar_msg(4, "#playpause.hook.2",
          "#previous.hook.1", "#next.hook.1",
          "#spotify.hook.1")) {
      CURRENT_SPOTIFY_STATE = PAUSED;
      return TRUE;
    }
  }
  return FALSE;
}

dbus_bool_t spotify_exited() {
  if (CURRENT_SPOTIFY_STATE != EXITED) {
    // Hide all buttons and track display on polybar
    if (polybar_msg(4, "#playpause.hook.0",
          "#previous.hook.0", "#next.hook.0",
          "#spotify.hook.0")) {
      CURRENT_SPOTIFY_STATE = EXITED;
      return TRUE;
    }
  }
  return FALSE;
}

dbus_bool_t polybar_msg(int numOfMsgs, ...) {
  va_list args;

  va_start(args, numOfMsgs);
  for (int m = 0; m < numOfMsgs; m++) {
    char *message = va_arg(args, char *);
    char *exec_args[]={"polybar-msg","action", message, NULL};

    // fork and exec polybar-msg calls for each variadic
    int pid = fork();
    if (pid < 0){
      printf("Fork error, unable to send message to polybar.\n");
      perror(errno);
      return FALSE;
    }
    else if (pid == 0){
      printf("%s%s%s\n", "Sending the message '", message, "' via "
          "polybar-msg.\n");

      execvp(exec_args[0], exec_args);

      printf("Execvp error, unable to send message to polybar.\n");
      perror(errno);
      return FALSE;
    }
    else {
      // Without sleep, requests are sometimes ignored
      // TODO Unsure if this is still needed. May leave this here since it
      // shouldn't negatively impact program execution.
      msleep(10);
    }
  }
  va_end(args);

  return TRUE;
}

DBusHandlerResult properties_changed_handler(DBusConnection *connection,
    DBusMessage *message,
    void *user_data) {
  if (VERBOSE) puts("Running properties_changed_handler");
  DBusMessageIter iter;
  DBusMessageIter sub_iter;
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
        dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_STRING) &&

      !(recurse_iter_of_type(&iter, &sub_iter, DBUS_TYPE_ARRAY) &&
        // Go to value with Metadata key
        iter_try_step_to_key(&sub_iter, "PlaybackStatus") &&
        // Step into variant value
        iter_try_step_into_type(&sub_iter, DBUS_TYPE_VARIANT) &&
        // Verify string type
        dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_STRING)) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  // Make sure trackid begins with spotify
  char *trackid = iter_get_string(&sub_iter);
  if (trackid != NULL && strncmp(trackid, "/com/spotify", 12) == 0) {
    spotify_update_sender(dbus_message_get_sender(message));
    spotify_update_track(trackid);
    update_last_trackid(trackid);
    is_spotify = TRUE;

    if (VERBOSE) puts("Spotify Detected");
  }

  free(trackid);

  if (!is_spotify && dbus_senderid && strcmp(dbus_senderid, dbus_message_get_sender(message)) == 0) {
    is_spotify = TRUE;
  }

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

  is_spotify = get_spotify_status();

  if (is_spotify) {
    update_last_trackid(get_now_playing());
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
  if (!dbus_connection_add_filter(connection, properties_changed_handler,
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
