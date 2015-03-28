all: bufcat

bufcat: libbufio
	gcc bufcat.c -L../lib -I../lib -Wl,-rpath=../lib -lbufio -o bufcat 

libbufio:
	make -C ../lib

clean:
	rm -f ./bufcat
