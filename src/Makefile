CFLAGS=-Wall
LIBS=-leibclient -lcrypto -lm
#LIBS=-lgmp -lcrypto -llibeibclient

all:	clean
	gcc $(CFLAGS) master.c sec.c clr.c knx.c cryptoHelper.c -o master -pthread $(LIBS)

debug: clean
	gcc $(CFLAGS) master.c clr.c sec.c knx.c cryptoHelper.c -o master -pthread -DDEBUG $(LIBS)

clean:
	clear
	rm -rf *.o
	rm -f master

drun:	debug
	@echo "starts master daemon in debug mode, with KNX device addr 1"
	../start.sh 1

run:	all
	../start.sh 1

update:	
	git commit -a --allow-empty
	git pull
	git push
