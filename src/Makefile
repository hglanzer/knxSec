CFLAGS=-Wall
#LIBS=-lcrypto
LIBS=-leibclient -lcrypto
#LIBS=-lgmp -lcrypto -llibeibclient

all:	clean update
	#gcc $(CFLAGS) $(LIBS) master.c sec.c clr.c knx.c -o master -pthread
	gcc $(CFLAGS) master.c sec.c clr.c knx.c -o master /usr/local/lib/libeibclient.so.0 -pthread

debug: clean
	gcc $(CFLAGS) $(LIBS) master.c clr.c sec.c knx.c -o master -pthread -DDEBUG
	#gcc $(CFLAGS) $(LIBS) master.c clr.c sec.c knx.c -o master /usr/local/lib/libeibclient.so.0 -pthread -DDEBUG

clean:
	clear
	rm -rf *.o
	rm -f master

run:	all
	./master

update:	
	git commit -a --allow-empty
	git pull
	git push
