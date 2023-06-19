CC=gcc
CFLAGS=-I. -lpcap
#CFLAGS=

OBJECTS = libpcap1


all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
