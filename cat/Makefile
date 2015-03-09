all: cat

cat: libhelpers
	gcc cat.c -L../lib -I../lib -Wl,-rpath=../lib -lhelpers -o cat 

libhelpers:
	make -C ../lib

clean:
	rm -f ./cat
