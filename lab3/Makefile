CC=gcc
CFLAGS=-I.
#CFLAGS=

OBJECTS = byteorder tcpcliv_1 tcpserv_1 test_signal echo_tcpcliv6 echo_tcpservv6 echo_tcpservv6_del daytimetcpcliv6_04 daytimetcpsrvv6_04  

DEPS = tcp_helper.h
OBJ1 = tcprecv01.o tcp_helper.o
OBJ2 = tcpsend01.o tcp_helper.o

all: $(OBJECTS) tcprecv01 tcpsend01

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)	
	
tcprecv01 : $(OBJ1)
	gcc -o $@ $^ $(CFLAGS)

tcpsend01 : $(OBJ2)
	gcc -o $@ $^ $(CFLAGS)
	
clean:
	rm  $(OBJECTS) *.o tcprecv01 tcpsend01
