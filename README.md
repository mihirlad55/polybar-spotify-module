# Polybar Spotify Player

This is a pure C implementation of an external polybar module that signals
polybar when a track is playing/paused. This is still a basic implementation
and lacks a status script and some polish.


# How it Works
The program connects to the DBus Session Bus and listens for two particular
signals:

- org.freedesktop.DBus.ProprtyChanged for track changes
- org.freedesktop.DBus.NameOwnerChanged for spotify disconnections

Using this information, it sends messages to spotify polybar custom/IPC modules
to show/hide spotify controls and display the play/pause icon based on whether
a song is playing/paused.


## Requirements
`dbus`


## Future Work
- Status program to retreive latest track information


## Why Did I Make this in C
- Practice/learn low-level C
- Reduce number of dependencies
- Make something as lightweight as possible by directly interfacing with DBus
- For fun 😜


## Credits
Inspired by the python implementation by
[dietervanhoof](https://github.com/dietervanhoof).
