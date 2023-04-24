CC=gcc

all: srvv6 cliv6

srvv6: daytimetcpsrvv6_02.c
	gcc -o srvv6 daytimetcpsrvv6_02.c

cliv6: daytimetcpcliv6_02.c
	gcc -o cliv6 daytimetcpcliv6_02.c

clean:
	rm srvv6 cliv6
