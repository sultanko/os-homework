DIRS = lib cat revwords filter \
	   bufcat simplesh filesender \
	   bipipe

all: 
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
