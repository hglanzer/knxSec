CFLAGS=-Wall
LIBS=-leibclient
#LIBS=-lgmp -lcrypto -llibeibclient

all:	clean update
	gcc $(CFLAGS) $(LIBS) master.c sec.c cls.c -o master -pthread
	#gcc $(CFLAGS) master.c sec.c cls.c -o master /usr/lib/libeibclient.so.0 -pthread

debug: clean
	gcc $(CFLAGS) master.c sec.c cls.c -o master /usr/lib/libeibclient.so.0 -pthread -DDEBUG 
	./master

clean:
	clear
	rm -rf *.o

run:	all
	./master

update:	
	git commit -a --allow-empty
	git pull
	git push
