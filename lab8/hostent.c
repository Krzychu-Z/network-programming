#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#if TIME_WITH_SYS_TIME
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include        <sys/time.h>    /* includes <time.h> unsafely */
#else
#include        <time.h>                /* old system? */
#endif
#endif
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 		<netdb.h>
#include 		<resolv.h>

int
main(int argc, char **argv)
{
	char			*ptr, **pptr;
	char			str[INET_ADDRSTRLEN];
	char			str6[INET6_ADDRSTRLEN];
	struct hostent	*hptr;

	res_init();
	if( argc < 2 ){
		exit(1);
	}
	if( argc == 3 ){
		_res.options |= RES_USE_INET6;
	}

//	while (--argc > 0) {
		ptr = *++argv;
		if ( (hptr = gethostbyname(ptr)) == NULL) {
			printf("gethostbyname error for host: %s: %s",
					ptr, hstrerror(h_errno));
//			continue;
			return(1);
		}
		printf("official hostname: %s\n", hptr->h_name);

		printf("h_length = %d\n", hptr->h_length);
		for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
			printf("\talias: %s\n", *pptr);

		switch (hptr->h_addrtype) {
		case AF_INET:
			pptr = hptr->h_addr_list;
			for ( ; *pptr != NULL; pptr++){
		      inet_ntop(AF_INET, (struct sockaddr  *) *pptr,  str, sizeof(str));
				printf("\taddress: %s\n", str);
			}
			break;

		case AF_INET6:
			pptr = hptr->h_addr_list;
			for ( ; *pptr != NULL; pptr++){
		      inet_ntop(AF_INET6, (struct sockaddr  *) *pptr,  str6, sizeof(str6));
				printf("\taddress: %s\n", str6);
			}
			break;
		default:
			fprintf(stderr,"unknown address type");
			break;
		}
//	}
	exit(0);
}

