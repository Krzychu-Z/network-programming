CC=gcc
CFLAGS=-I. -lsctp
#CFLAGS=

OBJECTS =  sctpclientv6_02  sctpservv6_06

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
