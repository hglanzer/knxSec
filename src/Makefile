CFLAGS=-Wall
LIBS=-leibclient -lcrypto
#LIBS=-lgmp -lcrypto -llibeibclient

all:	clean update
	gcc $(CFLAGS) master.c sec.c clr.c knx.c crypto.c -o master -pthread $(LIBS)

debug: clean
	gcc $(CFLAGS) master.c clr.c sec.c knx.c crypto.c -o master -pthread -DDEBUG $(LIBS)

clean:
	clear
	rm -rf *.o
	rm -f master

run:	all
	../start 1

update:	
	git commit -a --allow-empty
	git pull
	git push
