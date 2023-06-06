CC=gcc
CFLAGS=-I. -lsctp
#CFLAGS=

OBJECTS = sctpclientv4_01  sctpservv4_01 sctpservv4_fork_02

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
