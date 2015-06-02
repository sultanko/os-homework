CFLAGS = -Wall -std=gnu99 -g
LIB_DIR = ../lib
TARGET = forking polling
SOURCE = $(TARGET:%=%.c)

all: $(TARGET)

$(TARGET): libhelpers $(SOURCE)
	gcc $@.c -L$(LIB_DIR) -I$(LIB_DIR) -Wl,-rpath=$(LIB_DIR) $(CFLAGS) -lbufio -o $@

libhelpers:
	make -C $(LIB_DIR)

clean:
	rm -f ./$(TARGET)
	rm -f ./*.bin
