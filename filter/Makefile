all: filter

filter: libhelpers
	gcc filter.c -L../lib -I../lib -Wl,-rpath=../lib -lhelpers -o filter 

libhelpers:
	make -C ../lib

clean:
	rm -f ./filter
