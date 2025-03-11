CC=gcc
LDFLAGS=-pthread $(shell pkg-config --cflags --libs gtk+-3.0 libxml-2.0)
CFLAGS=-g -ggdb -Wall

all:
	${CC} ${CFLAGS} gtk-wrap.c -o gtk-wrap ${LDFLAGS}

clean:
	rm gtk-wrap

strip:
	strip -s gtk-wrap
