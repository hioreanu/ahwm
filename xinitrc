#!/bin/sh
xset s off
xhost localhost > /dev/null 2>&1
xhost 127.0.0.1 > /dev/null 2>&1
xsetroot -solid black
xmodmap ~/.xmodmap-laptop
xset dpms 6000 6002 6004
xsetroot -cursor_name left_ptr
exec xterm
xterm &

exec ./xwm

exit 1
