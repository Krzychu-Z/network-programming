CC=gcc
CFLAGS=-I. -lpthread -lm
#CFLAGS=

OBJECTS = echo_serv6_ws_epoll  tcpcliv6_select_echo 

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
