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

#include <sys/select.h>
#include <unistd.h>
#include <math.h>



#define MAXLINE 1024
#define SA      struct sockaddr


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

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = read(fd, ptr, nbytes)) == -1){
			perror("read error");
			exit(1);
	}
	return(n);
}

void
str_cli(FILE *fp, int sockfd)
{
	int			maxfdp1, stdineof;
	fd_set		rset;
	char		buf[MAXLINE];
	int		n;

	stdineof = 0;
	FD_ZERO(&rset);
	
	for ( ; ; ) {
		if (stdineof == 0){
			FD_SET(fileno(fp), &rset);
			printf("Wprowadz dane:"); fflush(stdout);
		}
			
		FD_SET(sockfd, &rset);
		maxfdp1 = fmax(fileno(fp), sockfd) + 1;
		
		if ( (n = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0){
				perror("select error");
				exit(1);
		}

		if (FD_ISSET(sockfd, &rset)) {	/* socket is readable */
			if ( (n = Read(sockfd, buf, MAXLINE)) == 0) {
				if (stdineof == 1)
					return;		/* normal termination */
				else{
					perror("str_cli: server terminated prematurely");
					exit(1);
				}
			}
			buf[n]=0;
			printf("\n Dane otrzymane od serwera: %s", buf); fflush(stdout);
		}

		if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			if ( (n = Read(fileno(fp), buf, MAXLINE)) == 0) {
				stdineof = 1;

				if (shutdown(sockfd, SHUT_WR) < 0){
					perror("shutdown error");
					exit(1);				
				}
				FD_CLR(fileno(fp), &rset);
				continue;
			}

			Writen(sockfd, buf, n);
			
		}
	}
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
		fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
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
