# Polybar Spotify Module

This is a pure C implementation of an external polybar module that signals
polybar when a track is playing/paused and when the track changes. There is
also a program that retreives the title and artist of the currently playing
song on spotify.


## Requirements
DBus is used for listening to spotify track changes. Obviously you need spotify
and polybar as well:
`dbus polybar spotify`

To compile the program, you will need `make`.


## How to Setup

### Installing the Program
Run the following code to clone the repo and install the programs.
```
git clone https://github.com/mihirlad55/polybar-spotify-module
cd polybar-spotify-module/src/
sudo make install
```

### Running spotify-listener in the Background
`spotify-listener` must run in the background for it to be able to listen for
track changes and communicate with polybar. There are two ways to keep it
running in the background. If you are using systemd, you can have it start at
system startup by running:

```
systemctl --user enable spotify-listener
```

Then, you can start the service now by running:

```
systemctl --user start spotify-listener
```

If you are not using systemd, make sure the `spotify-listener` program is
executed at startup. Something like
```
spotify-listener &
disown
```
in a startup script should accomplish that.


### Configuring Polybar
Your polybar configuration file should be located at `.config/polybar/config`

First make sure IPC is enabled. The following should be in your configuration
file under the `[bar/<your bar name>]` section:
```
[bar/main]
enable-ipc = true
```

If you plan to use icons for next/play/previous/pause buttons, make sure you
add (and have installed) an icon font accordingly such as NerdFonts or
FontAwesome under the `[bar/<your bar name>]` section:
```
[bar/main]
font-1 = Font Awesome 5 Free:size=10;1
```

Next, add the following spotify modules:
```
[module/previous]
type = custom/ipc
format-font = 2
; Default
hook-0 = echo ""
; When spotify active
hook-1 = echo ""
click-left = "spotifyctl -q previous"


[module/next]
type = custom/ipc
format-font = 2
; Default
hook-0 = echo ""
; When spotify active
hook-1 = echo ""
click-left = "spotifyctl -q next"


[module/playpause]
type = custom/ipc
format-font = 2
; Default
hook-0 = echo ""
; Playing
hook-1 = echo ""
; Paused
hook-2 = echo ""
click-left = "spotifyctl playpause"


[module/spotify]
type = custom/ipc
; Default
hook-0 = echo ""
; Playing/paused show status
hook-1 = spotifyctl -q status
```

The squares above are icons that can't be loaded. You can use whatever text or
symbols you want to substitute.

Lastly, make sure the new spotify modules are part of your bar. Make sure the
following is part of your modules.
```
modules-center = spotify previous playpause next
```

## How it Works
The spotify-listener program connects to the DBus Session Bus and listens for
two particular signals:

- `org.freedesktop.DBus.ProprtyChanged` for track changes
- `org.freedesktop.DBus.NameOwnerChanged` for spotify disconnections

Using this information, it sends messages to spotify polybar custom/IPC modules
to show/hide spotify controls and display the play/pause icon based on whether
a song is playing/paused.

The spotifyctl program calls `org.mpris.MediaPlayer2.Properties.Get` method to
retreive status information and calls methods in the
`org.mpris.MediaPlayer2.Player` interface to pause/play and go to the
previous/next track.


## Why Did I Make this in C
- Practice/learn low-level C
- Reduce number of dependencies
- Make something as lightweight as possible by directly interfacing with DBus
- For fun 😜


## Resources
The following are very useful resources for DBus API and specs:

- <https://dbus.freedesktop.org/doc/api/html/group__DBusConnection.html>
- <https://dbus.freedesktop.org/doc/api/html/group__DBusBus.html>
- <https://dbus.freedesktop.org/doc/api/html/group__DBusMessage.html>
- <https://dbus.freedesktop.org/doc/dbus-specification.html>
- <https://dbus.freedesktop.org/doc/dbus-tutorial.html>


## Credits
Inspired by the python implementation by
[dietervanhoof](https://github.com/dietervanhoof): <https://github.com/dietervanhoof/polybar-spotify-controls>
