# $Id$
SHELL=/bin/sh

CFLAGS=-DDEBUG=1 -I/usr/X11R6/include -L/usr/X11R6/lib -g
LIBS=-lX11

xwm: xwm.o client.o event.o focus.o workspace.o keyboard.o xev.o mouse.o cursor.o move-resize.o
	$(CC) $(CFLAGS) xev.o xwm.o client.o event.o focus.o workspace.o keyboard.o mouse.o cursor.o move-resize.o -o xwm $(LIBS)

.SUFFIXES: .c .o
.c.o: $*.h $*.c
	$(CC) $(CFLAGS) -c -o $*.o $<

dep:
	@-makedepend -Y *.c *.h > /dev/null 2>&1
	@-etags *.c *.h > /dev/null 2>&1
depend: dep
tags: dep

wordcount:
	@cat *.c *.h | fgrep -v '#include' | cpp -E - | egrep -v '^[ \t]*$$' | wc -l
wc: wordcount

clean:
	@rm -f *.o *~ *core xwm TAGS

# DO NOT DELETE

client.o: client.h xwm.h workspace.h keyboard.h cursor.h focus.h
cursor.o: cursor.h xwm.h ew_cursor_black.xbm ne_cursor_black.xbm
cursor.o: ns_cursor_black.xbm nw_cursor_black.xbm ew_cursor_white.xbm
cursor.o: ne_cursor_white.xbm ns_cursor_white.xbm nw_cursor_white.xbm
event.o: xwm.h event.h client.h focus.h workspace.h keyboard.h mouse.h xev.h
focus.o: focus.h client.h xwm.h workspace.h
keyboard-test.o: keyboard.h client.h xwm.h
keyboard.o: keyboard.h client.h xwm.h
mouse.o: mouse.h move-resize.h xwm.h cursor.h
move-resize.o: move-resize.h client.h xwm.h cursor.h event.h
workspace.o: workspace.h client.h xwm.h
xwm.o: xwm.h event.h client.h keyboard.h focus.h cursor.h mouse.h
xwm.o: move-resize.h
client.o: xwm.h
focus.o: client.h xwm.h
keyboard.o: client.h xwm.h
workspace.o: client.h xwm.h
