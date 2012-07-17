
INCLUDES=-I.

CFLAGS= -g -O2 -Wall $(INCLUDES)

OBJS=  getbits.o input.o getmesg.o main.o serv.o

acarsdec:	$(OBJS)
	$(CC) -o $@ $(OBJS) -lm -lsndfile -lasound

main.o:	main.c version.h

clean:
	rm -f *.o acarsdec

getbits.o: acarsdec.h getbits.c
getmesg.o: acarsdec.h getmesg.c
main.o: acarsdec.h main.c
input.o: acarsdec.h input.c
serv.o: acarsdec.h serv.c
