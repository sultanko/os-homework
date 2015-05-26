all: lib/libhelpers.so cat/cat revwords/revwords filter/filter \
	bufcat/bufcat simplesh/simplesh

lib/libhelpers.so:
	cd lib && make

cat/cat:
	cd cat && make

revwords/revwords:
	cd revwords && make

filter/filter:
	cd filter && make

bufcat/bufcat:
	cd bufcat && make

simplesh/simplesh:
	cd simplesh && make

clean:
	cd cat && make clean
	cd lib && make clean
	cd revwords && make clean
	cd filter && make clean
	cd bufcat && make clean
	cd simplesh && make clean
