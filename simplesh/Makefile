all: simplesh

simplesh: libhelpers 
	gcc simplesh.c -g -L../lib -I../lib -Wl,-rpath=../lib -std=gnu99 -lhelpers -lbufio -o simplesh 

libhelpers:
	make -C ../lib

clean:
	rm -f ./simplesh
