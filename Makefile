# $Id$
SHELL=/bin/sh

CFLAGS=-DSHAPE -O2 -I/usr/X11R6/include -L/usr/X11R6/lib
CFLAGS=-g -Wall -DSHAPE -DDEBUG=1 -I/usr/X11R6/include -L/usr/X11R6/lib
LIBS=-lX11 -lXext

OBJS=xwm.o client.o event.o focus.o workspace.o keyboard.o xev.o mouse.o cursor.o move-resize.o error.o kill.o malloc.o

all: xwm

xwm: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

.SUFFIXES: .c .o
.c.o:
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
	@rm -f *.o *~ *core xwm # TAGS

# DO NOT DELETE

client.o: client.h xwm.h workspace.h keyboard.h cursor.h focus.h malloc.h
cursor.o: cursor.h xwm.h malloc.h ew_cursor_black.xbm ne_cursor_black.xbm
cursor.o: ns_cursor_black.xbm nw_cursor_black.xbm ew_cursor_white.xbm
cursor.o: ne_cursor_white.xbm ns_cursor_white.xbm nw_cursor_white.xbm
error.o: error.h xwm.h
event.o: xwm.h event.h client.h focus.h workspace.h keyboard.h mouse.h xev.h
event.o: error.h malloc.h
focus.o: focus.h client.h xwm.h workspace.h
keyboard-test.o: keyboard.h client.h xwm.h
keyboard.o: keyboard.h client.h xwm.h malloc.h
kill.o: kill.h client.h xwm.h event.h
malloc.o: malloc.h
mouse.o: mouse.h client.h xwm.h move-resize.h cursor.h malloc.h
move-resize.o: move-resize.h client.h xwm.h cursor.h event.h malloc.h
workspace.o: workspace.h client.h xwm.h
xev.o: malloc.h
xwm.o: xwm.h event.h client.h keyboard.h focus.h cursor.h mouse.h
xwm.o: move-resize.h error.h kill.h
client.o: xwm.h
focus.o: client.h xwm.h
keyboard.o: client.h xwm.h
kill.o: client.h xwm.h
mouse.o: client.h xwm.h
workspace.o: client.h xwm.h
