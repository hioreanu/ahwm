# $Id$
SHELL=/bin/sh

CFLAGS=-DSHAPE -O2 -I/usr/X11R6/include -L/usr/X11R6/lib
CFLAGS=-g -Wall -DSHAPE -DDEBUG=1 -I/usr/X11R6/include -L/usr/X11R6/lib
LIBS=-lX11 -lXext

OBJS=xwm.o client.o event.o focus.o workspace.o keyboard.o xev.o mouse.o cursor.o move-resize.o error.o kill.o malloc.o icccm.o colormap.o ewmh.o debug.o

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

client.o: client.h xwm.h workspace.h keyboard.h mouse.h cursor.h focus.h
client.o: event.h malloc.h debug.h
colormap.o: colormap.h client.h xwm.h
cursor.o: cursor.h xwm.h
debug.o: debug.h
error.o: error.h xwm.h
event.o: xwm.h event.h client.h focus.h workspace.h keyboard.h mouse.h xev.h
event.o: error.h malloc.h move-resize.h debug.h ewmh.h
ewmh.o: xwm.h ewmh.h client.h malloc.h debug.h
focus.o: focus.h client.h xwm.h workspace.h debug.h event.h
icccm.o: xwm.h icccm.h
keyboard-test.o: keyboard.h client.h xwm.h
keyboard.o: keyboard.h client.h xwm.h malloc.h
kill.o: kill.h client.h xwm.h event.h debug.h
malloc.o: malloc.h
mouse.o: mouse.h client.h xwm.h move-resize.h cursor.h malloc.h
move-resize.o: move-resize.h client.h xwm.h cursor.h event.h malloc.h debug.h
workspace.o: workspace.h client.h xwm.h focus.h event.h debug.h
xev.o: malloc.h
xwm.o: xwm.h event.h client.h keyboard.h focus.h workspace.h cursor.h mouse.h
xwm.o: move-resize.h error.h kill.h icccm.h ewmh.h
client.o: xwm.h
colormap.o: client.h xwm.h
ewmh.o: client.h xwm.h
focus.o: client.h xwm.h workspace.h
keyboard.o: client.h xwm.h
kill.o: client.h xwm.h
mouse.o: client.h xwm.h
workspace.o: client.h xwm.h
