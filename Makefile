# $Id$
SHELL=/bin/sh

CFLAGS=-DSHAPE -O2 -I/usr/X11R6/include -L/usr/X11R6/lib
CFLAGS=-g -Wall -DSHAPE -DDEBUG=1 -I/usr/X11R6/include -L/usr/X11R6/lib
LIBS=-lX11 -lXext

OBJS=xwm.o client.o event.o focus.o workspace.o keyboard-mouse.o xev.o cursor.o move-resize.o error.o kill.o malloc.o icccm.o colormap.o ewmh.o debug.o place.o

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

client.o: client.h xwm.h workspace.h keyboard-mouse.h cursor.h focus.h
client.o: event.h malloc.h debug.h ewmh.h move-resize.h
colormap.o: colormap.h client.h xwm.h
cursor.o: cursor.h xwm.h
debug.o: debug.h
error.o: error.h xwm.h
event.o: xwm.h event.h client.h focus.h workspace.h keyboard-mouse.h xev.h
event.o: error.h malloc.h move-resize.h debug.h ewmh.h place.h
ewmh.o: xwm.h ewmh.h client.h malloc.h debug.h focus.h workspace.h
focus.o: focus.h client.h xwm.h workspace.h debug.h event.h ewmh.h
icccm.o: xwm.h icccm.h
keyboard-mouse.o: keyboard-mouse.h client.h xwm.h malloc.h workspace.h
keyboard-mouse.o: debug.h event.h focus.h cursor.h
kill.o: kill.h client.h xwm.h event.h debug.h
malloc.o: malloc.h
move-resize.o: move-resize.h client.h xwm.h cursor.h event.h malloc.h debug.h
place.o: place.h client.h xwm.h workspace.h debug.h
workspace.o: workspace.h client.h xwm.h focus.h event.h debug.h ewmh.h
xev.o: malloc.h
xwm.o: xwm.h event.h client.h keyboard-mouse.h focus.h workspace.h cursor.h
xwm.o: move-resize.h error.h kill.h icccm.h ewmh.h
client.o: xwm.h
colormap.o: client.h xwm.h
ewmh.o: client.h xwm.h
focus.o: client.h xwm.h workspace.h
keyboard-mouse.o: client.h xwm.h
kill.o: client.h xwm.h
place.o: client.h xwm.h
workspace.o: client.h xwm.h
