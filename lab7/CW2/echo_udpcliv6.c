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

#define MAXLINE 1024
#define SA      struct sockaddr


char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		perror("fgets error");

	return (rptr);
}

void
echo_cli(int sockfd, const SA *pservaddr, socklen_t servlen, FILE *fp)
{
	int				n, i;
	char			sendline[MAXLINE], recvline[MAXLINE + 1];
	socklen_t		len;
	struct sockaddr	preply_addr;
	char			str[INET6_ADDRSTRLEN+1];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	struct timeval delay;
	
	bzero( sendline, sizeof(sendline));
	delay.tv_sec =2;  //opozienie na gniezdzie
	delay.tv_usec = 1; 
	len = sizeof(delay);
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
		fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
		return;
	}
	printf("\n>");
	while (Fgets(sendline, MAXLINE, fp) != NULL) {
	
		len = servlen;
		for(i=0; i < 3 ; i++ ){

			if( sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen) <0 ){
				perror("sendto error");
				exit(1);
			}

			if( (n = recvfrom(sockfd, recvline, MAXLINE, 0, &preply_addr, &len) ) < 0 ){
				perror("recfrom error");
				if( (errno == (EAGAIN | EWOULDBLOCK)) && (i < 2) )
					continue;
				else{
					exit(1);
				}
			}else
				break;
		}
	
		bzero(str, sizeof(str));
	
 		if( preply_addr.sa_family == AF_INET6 ){
		      cliaddr = (struct sockaddr_in6*) &preply_addr;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  str, sizeof(str));
		}
		else{
		      cliaddrv4 = (struct sockaddr_in*) &preply_addr;
		      inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  str, sizeof(str));
		}

		printf("Echo from %s (%d bytes)\n", str,n);

		if (len != servlen || memcmp(pservaddr, &preply_addr, len) != 0) {
			printf("reply from %s (ignored)\n", str);
		}

		recvline[n] = 0;	/* null terminate */
		if (fputs(recvline, stdout) == EOF){
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			exit(1);
		}
		printf(">");
	}
}


int
main(int argc, char **argv)
{
	int					sockfd, n, delay;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];

	if (argc != 2){
		fprintf(stderr, "usage: %s <IPaddress> \n", argv[0]);
		return 1;
	}
	if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(7);	/* echo server */
	if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}
	
	echo_cli( sockfd, (SA *) &servaddr, sizeof(servaddr), stdin);
	
	exit(0);
}
