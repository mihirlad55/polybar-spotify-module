#ifndef _SPOTIFY_LISTENER_H_
#define _SPOTIFY_LISTENER_H_

#include <dbus-1.0/dbus/dbus.h>
#include <stdarg.h>

/**
 * Send the specified messages to polybar through IPC
 *
 * @param int numOfMsgs Number of variadic messages that will be given as args
 * @param ... char* strings to send
 *
 * @returns dbus_bool_t TRUE if messages successfully sent, FALSE otherwise.
 */
dbus_bool_t polybar_msg(int numOfMsgs, ...);

/**
 * DBus handler function for PropertiesChanged signals. This is automatically
 * called by DBus when a PropertiesChanged signal is broadcasted.
 *
 * @param DBusConnection* connection The DBusConnection object
 * @param DBusMessage* message The PropertiesChanged signal message
 * @param void *user_data Pointer to extra user data for handler functions. Not
 *                        used.
 *
 * @returns DBusHandlerResult The result of handling the signal. This returns
 * DBUS_HANDLER_RESULT_HANDLED if it was a signal from spotify contianing the
 * desired information, otherwise returns DBUS_HANDLER_RESULT_NOT_YET_HANDLED.
 */
DBusHandlerResult properties_changed_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data);

/**
 * DBus handler function for NameOwnerChanged signals. This is automatically
 * called by DBus when a NameOwnerChanged signal is broadcasted.
 *
 * @param DBusConnection* connection The DBusConnection object
 * @param DBusMessage* message The NameOwnerChanged signal message
 * @param void *user_data Pointer to extra user data for handler functions. Not
 *                        used.
 *
 * @returns DBusHandlerResult The result of handling the signal. This returns
 * DBUS_HANDLER_RESULT_HANDLED if it was a signal from spotify indicating a
 * disconnection, otherwise returns DBUS_HANDLER_RESULT_NOT_YET_HANDLED.
 */
DBusHandlerResult name_owner_changed_handler(DBusConnection *connection,
                                             DBusMessage *message,
                                             void *user_data);

/**
 * Dummy function used by DBus Handlers for extra user data. Since the handler
 * functions being used do not use any extra data, this function does nothing.
 *
 * @param void* memory Pointer to memory passed to the handler function to be
 * freed.
 */
void free_user_data(void *memory);

/**
 * Updates current stored spotify state and sends IPC messages to polybar to
 * update the spotify modules. This function does nothing if the current stored
 * state is already PLAYING.
 *
 * @returns dbus_bool_t Returns TRUE if the state was changed, and FALSE
 *                      otherwise.
 */
dbus_bool_t spotify_playing();

/**
 * Updates current stored spotify state and sends IPC messages to polybar to
 * update the spotify modules. This function does nothing if the current stored
 * state is already PAUSED.
 *
 * @returns dbus_bool_t Returns TRUE if the state was changed, and FALSE
 *                      otherwise.
 */
dbus_bool_t spotify_paused();

/**
 * Updates current stored spotify state and sends IPC messages to polybar to
 * update the spotify modules. This function does nothing if the current stored
 * state is already EXITED.
 *
 * @returns dbus_bool_t Returns TRUE if the state was changed, and FALSE
 *                      otherwise.
 */
dbus_bool_t spotify_exited();

/**
 * Updates the currently stored spotify trackid.
 *
 * @param const char* The trackid
 *
 * @returns TRUE if no error, otherwise FALSE.
 */
dbus_bool_t update_last_trackid(const char *trackid);

/**
 * If the trackid has changed, an IPC message is sent to polybar to the status
 * module indicating a track change.
 *
 * @param const char* current_trackid The current trackid reported by spotify
 *
 * @returns dbus_bool_t TRUE if the trackid has changed, FALSE otherwise
 */
dbus_bool_t spotify_update_track(const char *current_trackid);

#endif
