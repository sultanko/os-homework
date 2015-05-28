all: libhelpers.so libbufio.so

libhelpers.so: helpers.o
	gcc -shared helpers.o -o libhelpers.so

helpers.o:
	gcc -c -g -fpic -std=gnu99 helpers.c -o helpers.o

libbufio.so: bufio.o
	gcc -shared bufio.o -o libbufio.so

bufio.o:
	gcc -c -g -fpic -std=gnu99 bufio.c -o bufio.o

clean:
	rm -f *.o
	rm -f *.so
