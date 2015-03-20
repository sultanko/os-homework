all: revwords

revwords: libhelpers
	gcc delwords.c -L../lib -I../lib -Wl,-rpath=../lib -lhelpers -o delwords 

libhelpers:
	make -C ../lib

clean:
	rm -f ./delwords
