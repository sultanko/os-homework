CFLAGS = -Wall -std=gnu99 -g
LIB_DIR = ../lib
TARGET = simplesh

all: $(TARGET)

$(TARGET): libhelpers 
	gcc $@.c -L$(LIB_DIR) -I$(LIB_DIR) -Wl,-rpath=$(LIB_DIR) $(CFLAGS) -lbufio -lhelpers -o $@ 

libhelpers:
	make -C $(LIB_DIR)

clean:
	rm -f ./$(TARGET)
