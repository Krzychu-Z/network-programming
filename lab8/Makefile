CC=gcc
CFLAGS=-I. 
#CFLAGS=

OBJECTS = daytimetcpsrvv6  getaddrinfo readdir udp_echo_cli_getaddrinfo daytimetcpcli_name daytimetcpsrvv6_daemon hostent  syslog_test udp_echo_serv_getaddrinfo


all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
