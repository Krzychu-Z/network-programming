CC=gcc
CFLAGS=-I. -lm
#CFLAGS=

OBJECTS = tcpcliv6_echo tcpcliv6_select_echo  tcpserv6_ws_echo_fork  tcpserv6_ws_echo_poll tcpserv6_ws_echo_select

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
