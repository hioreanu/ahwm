#!/bin/sh
xset s off
xhost localhost > /dev/null 2>&1
xhost 127.0.0.1 > /dev/null 2>&1
xsetroot -solid darkslategray
xmodmap ~/.xmodmap-laptop
#xmodmap ./xmodmap
xrdb -merge < ./Xdefaults
xset dpms 6000 6002 6004
xsetroot -cursor_name left_ptr
#exec xterm
#xterm &
#sleep 1

#exec wmaker
#exec kwin
#exec startkde
#exec ./9wm
#exec enlightenment
#exec fvwm2
#exec twm
#exec ctwm
export MALLOC_CHECK_=2
exec ./xwm

exit 1
