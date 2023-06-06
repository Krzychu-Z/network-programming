CC=gcc
CFLAGS=-I. -lsctp
#CFLAGS=

OBJECTS = daytimesctpcliv6_04  daytimesctpsrvv6_04  daytimetcpcliv4  daytimetcpcliv6_04  daytimetcpcliv6  daytimetcpsrvv4  daytimetcpsrvv6_04 daytimetcpsrvv6

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
