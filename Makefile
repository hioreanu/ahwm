# $Id$
SHELL=/bin/sh

CFLAGS=-I/usr/X11R6/include -L/usr/X11R6/lib -g
LIBS=-lX11

xwm: xwm.o client.o event.o focus.o workspace.o
	$(CC) $(CFLAGS) xwm.o client.o event.o focus.o workspace.o -o xwm $(LIBS)

.SUFFIXES: .c .o
.c.o: $*.h $*.c
	$(CC) $(CFLAGS) -c -o $*.o $<

dep:
	@-makedepend -Y *.c *.h > /dev/null 2>&1
	@-etags *.c *.h > /dev/null 2>&1
depend: dep
tags: dep

clean:
	@rm -f *.o *~ *core xwm TAGS

# DO NOT DELETE

client.o: client.h xwm.h workspace.h
event.o: xwm.h event.h client.h
focus.o: focus.h client.h xwm.h workspace.h
workspace.o: workspace.h client.h xwm.h
xwm.o: xwm.h event.h client.h
client.o: xwm.h
focus.o: client.h xwm.h
workspace.o: client.h xwm.h
