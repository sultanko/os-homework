all: revwords

revwords: libhelpers
	gcc revwords.c -L../lib -I../lib -Wl,-rpath=../lib -lhelpers -o revwords 

libhelpers:
	make -C ../lib

clean:
	rm -f ./revwords
