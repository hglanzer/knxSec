CFLAGS=-Wall
LIBS=-lgmp -lcrypto

all:	clean
	gcc $(CFLAGS) master.c sec.c cls.c -o master /usr/lib/libeibclient.so.0 -pthread

debug: clean
	gcc $(CFLAGS) master.c sec.c cls.c -o master /usr/lib/libeibclient.so.0 -pthread -DDEBUG
	./master

clean:
	clear
	rm -rf *.o

run:	all
	./master
