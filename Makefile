# $Id$
SHELL=/bin/sh

CFLAGS=-DDEBUG=1 -I/usr/X11R6/include -L/usr/X11R6/lib -g
LIBS=-lX11

xwm: xwm.o client.o event.o focus.o workspace.o keyboard.o xev.o mouse.o
	$(CC) $(CFLAGS) xev.o xwm.o client.o event.o focus.o workspace.o keyboard.o mouse.o -o xwm $(LIBS)

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

client.o: client.h xwm.h workspace.h keyboard.h
event.o: xwm.h event.h client.h workspace.h keyboard.h
focus.o: focus.h client.h xwm.h workspace.h
icccm.o: icccm.h client.h xwm.h
keyboard-test.o: keyboard.h client.h xwm.h
keyboard.o: keyboard.h client.h xwm.h
workspace.o: workspace.h client.h xwm.h
xwm.o: xwm.h event.h client.h keyboard.h focus.h
client.o: xwm.h
focus.o: client.h xwm.h
icccm.o: client.h xwm.h
keyboard.o: client.h xwm.h
workspace.o: client.h xwm.h
