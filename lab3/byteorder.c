#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int
main(int argc, char **argv)
{
 	union {
 	     short s;
 	     char c[sizeof(short)];
 	} un;
 	un.s = 0x0102;
//	un.s = htons(un.s);
 	printf("\n");
	system("uname -a");
 	if (sizeof(short) == 2) {
 		if (un.c[0] == 1 && un.c[1] == 2)
 			printf("Byte order: big-endian\n\n");
 	else if (un.c[0] == 2 && un.c[1] == 1)
 		printf("Byte order: little-endian\n\n");
 	         else
 		printf("unknown\n\n");
 	} else
 	         printf("sizeof(short) = %lu\n", sizeof(short));

 	exit(0);
}

