//#include	"unp.h"
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
#include 		<unistd.h>

#define MAXLINE 1024
#define SA      struct sockaddr



void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		perror("fputs error");
}


char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		perror("fgets error");

	return (rptr);
}


ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		perror("writen error");
}

void
str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];
	int n;

	printf("\n>");
	while (Fgets(sendline, MAXLINE, fp) != NULL) {

		Writen(sockfd, sendline, strlen(sendline));

		if ((n=read(sockfd, recvline, MAXLINE)) == 0){
			perror("str_cli: server terminated prematurely");
			exit(0);
		}
		recvline[n]=0;
		printf(" Dane otrzymane od serwera: ");
		Fputs(recvline, stdout);
		printf("\n>");
	}

	return;
}


int
main(int argc, char **argv)
{
	int					sockfd, n;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];

	if (argc != 2){
		fprintf(stderr, "usage: %s <IPaddress> \n", argv[0]);
		return 1;
	}
	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(7);	/* echo server */
	if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0){
		fprintf(stderr,"Address error: inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		return 1;
	}


	str_cli(stdin, sockfd);		/* do it all */

	fprintf(stderr,"\nNormalne zakonczenie programu %s.\n", argv[0]);
	fflush(stderr);

	exit(0);
}
