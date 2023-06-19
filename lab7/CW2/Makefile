CC=gcc
CFLAGS=-I. -lpthread -lm
#CFLAGS=

OBJECTS = echo_serv6_ws_epoll  echo_serv6_ws_poll echo_udpcliv6 tcpcliv6_select_echo  tcpserv6_ws_echo_fork

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
