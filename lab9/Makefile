CC=gcc
CFLAGS=-I. 
#CFLAGS=

OBJECTS = if_index  pf_packet_rcv_v3 pf_packet_rcv_v2 pf_packet_snd_v2


all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
