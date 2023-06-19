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
#include	<limits.h>		/* for OPEN_MAX */
#include 	<poll.h>
#include 	<unistd.h>
 
#define MAXLINE 1024
#define SA struct sockaddr
#define LISTENQ 2
#define INFTIM -1


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


int
main(int argc, char **argv)
{
	int		listenfd, connfd, sockfd;
	pid_t		childpid;
	socklen_t	clilen;
	struct sockaddr_in6	cliaddr, servaddr;
	void		sig_chld(int);
	int		i, maxi, maxfd, n; 
	int		nready;
	char 		buf[MAXLINE], addr_buf[INET6_ADDRSTRLEN+1];
	struct pollfd	client[FOPEN_MAX];



	if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
	       fprintf(stderr,"socket error : %s\n", strerror(errno));
	       return 1;
	}


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(7);	/* echo server */

	if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
	        fprintf(stderr,"bind error : %s\n", strerror(errno));
	        return 1;
	}
	
	if ( listen(listenfd, LISTENQ) < 0){
	        fprintf(stderr,"listen error : %s\n", strerror(errno));
	        return 1;
	}


	client[0].fd = listenfd;
	client[0].events = POLLIN;
	for (i = 1; i < FOPEN_MAX; i++)
		client[i].fd = -1;		/* -1 indicates available entry */
	maxi = 0;					/* max index into client[] array */

	for ( ; ; ) {
		if ( (nready = poll(client, maxi+1, INFTIM)) < 0){
				perror("poll error");
				exit(1); 					//change to something more intelligent
		}
		if (client[0].revents & POLLIN) {	/* new client connection */
			clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
				perror("accept error");
				exit(1);
		}


		   bzero(addr_buf, sizeof(addr_buf));
	   	inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  addr_buf, sizeof(addr_buf));
			printf("new client: %s, port %d\n",	addr_buf, ntohs(cliaddr.sin6_port));

			for (i = 1; i < FOPEN_MAX; i++)
				if (client[i].fd < 0) {
					client[i].fd = connfd;	/* save descriptor */
					break;
				}
			if (i == FOPEN_MAX){
				perror("too many clients");
				continue;
			}

			client[i].events = POLLIN;
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 1; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i].fd) < 0)
				continue;
			if (client[i].revents & (POLLIN | POLLERR)) {
				if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
					if (errno == ECONNRESET) {
							/*4connection reset by client */
						printf("client[%d] aborted connection\n", i);
						close(sockfd);
						client[i].fd = -1;
					} else{
						perror("read error");
						exit(1);
					}
				} else if (n == 0) {
						/*4connection closed by client */
					printf("client[%d] closed connection\n", i);
					close(sockfd);
					client[i].fd = -1;
				} else
					Writen(sockfd, buf, n);

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
