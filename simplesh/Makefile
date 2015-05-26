all: simplesh

simplesh: libhelpers
	gcc simplesh.c -g -L../lib -I../lib -Wl,-rpath=../lib -std=c99 -lhelpers -o simplesh 

libhelpers:
	make -C ../lib

clean:
	rm -f ./simplesh
