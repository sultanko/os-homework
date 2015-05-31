all: simplesh

filesender: libhelpers 
	gcc filesender.c -g -L../lib -I../lib -Wl,-rpath=../lib -std=gnu99 -lbufio -o filesender 

libhelpers:
	make -C ../lib

clean:
	rm -f ./filesender
