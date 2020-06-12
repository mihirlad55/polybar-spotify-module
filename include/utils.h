#ifndef _UTILS_H_
#define _UTILS_H_

#include <dbus-1.0/dbus/dbus.h>

/**
 * Get the string pointed to by a DBusMessageIter
 *
 * @param DBusMessageIter* The iterator pointing to the string
 *
 * @returns char* The string pointed to by the iter if it is pointing at a
 *                string. Otherwise, it returns a NULL pointer. This pointer
 *                must be freed by the caller.
 */
char *iter_get_string(DBusMessageIter *iter);

/**
 * Prints the string pointed to by a DBusMessageIter
 *
 * @param DBusMessage* iter The iterator pointing to a string
 */
void print_string_iter(DBusMessageIter *iter);

/**
 * Initialize subiter inside the container pointed to be iter if it is of the
 * specified type.
 *
 * @param DBusMessageIter* iter The iterator pointing at a container
 * @param DBusMessageIter* subiter The iterator to initialize inside the
 *                                 container.
 * @param int type The exptected type of the container
 *
 * @returns dbus_bool_t Returns TRUE if the specified type matches the type of
 *                      the container and the subiter is successfully
 *                      initialized, otherwise returns FALSE.
 */
dbus_bool_t recurse_iter_of_type(DBusMessageIter *iter,
                                 DBusMessageIter *subiter, int type);

/**
 * Initialize subiter inside the container pointed to be iter if it is of the
 * specified signature.
 *
 * @param DBusMessageIter* iter The iterator pointing at a container
 * @param DBusMessageIter* subiter The iterator to initialize inside the
 *                                 container.
 * @param const char* signature The exptected signature of the container
 *
 * @returns dbus_bool_t Returns TRUE if the specified signature matches the
 *                      signature of the container and the subiter is
 *                      successfully initialized, otherwise returns FALSE.
 */
dbus_bool_t recurse_iter_of_signature(DBusMessageIter *iter,
                                      DBusMessageIter *subiter,
                                      const char *signature);

/**
 * Initialize entry_iter at an array dict element matching the specified key.
 *
 * @param DBusMessageIter* array_iter The iterator pointing at an array of
 *                                    dictionary entries.
 * @param DBusMessageIter* entry_iter The iterator to initialize at the value of
 *                                    the dictionary entry.
 * @param const char* key The key of the dictionary entry.
 *
 * @returns dbus_bool_t Returns TRUE if a dictionary entry with the specified
 *                      key is found and the entry_iter is successfully
 *                      initialized, otherwise returns FALSE.
 */
dbus_bool_t iter_go_to_key(DBusMessageIter *array_iter,
                           DBusMessageIter *entry_iter, const char *key);

/**
 * Try to recurse the iter into a container that it is pointing at with the
 * specified type. This points the iter to the first element of the container,
 * rather than initializing a new subiter.
 *
 * @param DBusMessageIter* iter The iterator pointing at a container
 * @param int type The exptected type of the container
 *
 * @returns dbus_bool_t Returns TRUE if the specified type matches the type of
 *                      the container and the iter is successfully inside the
 *                      container, otherwise returns FALSE.
 */
dbus_bool_t iter_try_step_into_type(DBusMessageIter *iter, int type);

/**
 * Try to recurse the iter into a container that it is pointing at with the
 * specified signature. This points the iter to the first element of the
 * container, rather than initializing a new subiter.
 *
 * @param DBusMessageIter* iter The iterator pointing at a container
 * @param const char* signature The exptected signature of the container
 *
 * @returns dbus_bool_t Returns TRUE if the specified signature matches the
 *                      signature of the container and the iter is successfully
 *                      initialized inside the container, otherwise returns
 *                      FALSE.
 */
dbus_bool_t iter_try_step_into_signature(DBusMessageIter *iter,
                                         const char *signature);

/**
 * Try to recurse the iter into an array of dictionary entries and set the iter
 * to point at the value of a dictionary entry of the specified key. This points
 * the iter to the value of the dictionary entry, rather than initializing a new
 * subiter.
 *
 * @param DBusMessageIter* iter The iterator pointing at an array of dictionary
 *                              entries.
 * @param const char* key The key of desired dictionary entry
 *
 * @returns dbus_bool_t Returns TRUE if a dictionary entry is found with the
 *                      specified key and the iter is successfully initialized
 *                      at the value of this dictionary entry, otherwise
 *                      returns FALSE.
 */
dbus_bool_t iter_try_step_to_key(DBusMessageIter *element_iter,
                                 const char *key);

/**
 * Sleep milliseconds
 *
 * @param long milliseconds The number of milliseconds to sleep
 *
 * @returns dbus_bool_t Returns TRUE if the function is able to sleep,
 *                      successfully, otherwise FALSE.
 */
dbus_bool_t msleep(long milliseconds);

/**
 * Get an array of paths to polybar's IPC files in the specified directory.
 *
 * @param const char* ipc_path The directory in which to search for IPC files
 * @param char*** ptr_paths A pointer to a pointer to an array of paths that
 *                          will be populated with the paths to the found IPC
 *                          files.
 * @param size_t* num_of_paths The number of paths found by the function (i.e.
 *                             the number of elements in **ptr_paths).
 *
 * @returns dbus_bool_t Returns TRUE if the directory is successfully searched
 *                      for paths, otherwise FALSE.
 */
dbus_bool_t get_polybar_ipc_paths(const char *ipc_path, char **ptr_paths[],
                                  size_t *num_of_paths);

/**
 * Join two paths together. This function takes into account if the first path
 * ends in a '/' and concatenates the two paths together.
 * TODO: normalize paths instead of checking for '/'
 *
 * @param const char* p1 The first path
 * @param const char* p2 The second path (i.e. the path to concatenate)
 *
 * @returns char* The path with p1 and p2 joined together and NULL on error.
 *                This pointer must be freed by the caller.
 */
char *join_path(const char *p1, const char *p2);

/**
 * Replace all instances of a given string in a source string with another
 * string. This works from left-to-right, replacing 'strstr' with 'test' in the
 * string 'strstrstrstr' would result in 'testtest'.
 *
 * @param char* str The string to make the replacements in
 * @param char* find The string to look for
 * @param char* repl The string to replace `find` with in `str`
 *
 * @returns char* A new string with the replacements made if any.
 */
char* str_replace_all(char *str, char *find, char *repl);

/**
 * Truncate the specified string if it longer than the specified maximum length
 * and end the string with trunc while satisfying the max length constraint.
 *
 * @param char* str The string to truncate
 * @param const int max_len The maximum length that this string should be
 * @param char* trunc The string to end str with if its being truncated.
 *
 * @returns char* The truncated string. If str is longer than max_len, str will
 * be cut off at max_len, and the end of the string will be replaced with trunc.
 */
char* str_trunc(char *str, const int max_len, char *trunc);

/**
 * Find the number of matches of a given string in another string. This works
 * from left-to-right using similar logic to the str_replace_all function. As a
 * result, searching for 'strstr' in 'strstrstrstr' would result in 2 matches,
 * not 3 (the middle strstr).
 *
 * @param char* str The source string to search in
 * @param char* find The string to search for
 *
 * @returns int The number of matches found
 */
int num_of_matches(char *str, char *find);

#endif
