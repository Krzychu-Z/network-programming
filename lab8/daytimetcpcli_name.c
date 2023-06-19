#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 	<netdb.h>
#include 	<resolv.h>
#include 	<unistd.h>

#define MAXLINE 1024
#define SA      struct sockaddr



int
main(int argc, char **argv)
{
	int					sockfd, n;
	char				recvline[MAXLINE + 1];
	struct sockaddr_in	servaddr;
	struct sockaddr_in6	servaddrv6;
	struct in6_addr		*inetaddrp[2];
	char				**pptr;
	struct in6_addr		inetaddr;
	struct hostent		*hp;
	struct hostent		m_hp;
	struct servent		*sp;
	char			str6[INET6_ADDRSTRLEN];

	if (argc != 3){
		printf("\nusage: daytimetcpcli1 <hostname> <service>\n");
		exit(1);
	}
	
	if ( (hp = gethostbyname(argv[1])) == NULL) {

		if (inet_pton(AF_INET6, argv[1], &inetaddr) <= 0){
			printf("gethostbyname error for host: %s: %s\n",
					argv[1], hstrerror(h_errno));
			return 1;
		}
		inetaddrp[0] = &inetaddr;
		inetaddrp[1] = NULL;
		m_hp.h_addr_list = (char **) inetaddrp;
		m_hp.h_addrtype = AF_INET6;
		m_hp.h_name = argv[1];
		hp=&m_hp;
	} 

	if ( (sp = getservbyname(argv[2], "tcp")) == NULL){
		printf("getservbyname error for %s", argv[2]);
		return 2;
	}
	
	printf("Official hostname: %s\n", hp->h_name);

	switch (hp->h_addrtype) {
		case AF_INET:
			pptr = hp->h_addr_list;

			if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return 2;
			}

			for ( ; *pptr != NULL; pptr++){

				bzero(&servaddr, sizeof(servaddr));
				servaddr.sin_family = AF_INET;
				servaddr.sin_port = sp->s_port;
				memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));
		      	
		      	inet_ntop(AF_INET, (struct sockaddr  *) *pptr,  str6, sizeof(str6));
				printf("\ttrying: %s\n", str6);

				if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) == 0)
					break;		/* success */
				fprintf(stderr,"IPV4 connect error : %s \n", strerror(errno));
				close(sockfd);
			}
			break;

		case AF_INET6:
			pptr = hp->h_addr_list;
			if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
				fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
				return 1;
			}
			for ( ; *pptr != NULL; pptr++){
				bzero(&servaddrv6, sizeof(servaddrv6));
				servaddrv6.sin6_family = AF_INET6;
				servaddrv6.sin6_port = sp->s_port;
				memcpy(&servaddrv6.sin6_addr, *pptr, sizeof(struct in6_addr));
		      	
		      	inet_ntop(AF_INET6, (struct sockaddr  *) *pptr,  str6, sizeof(str6));
				printf("\ttrying: %s\n", str6);
				
				if (connect(sockfd, (SA *) &servaddrv6, sizeof(servaddrv6)) == 0)
					break;
				fprintf(stderr,"IPV6 connect error : %s \n", strerror(errno));
				close(sockfd);
			}
			break;
		default:
			fprintf(stderr,"unknown address type\n");
			break;
	}


	if (*pptr == NULL){
		printf("unable to connect\n");
		return 1;
	}
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}
	exit(0);
}
