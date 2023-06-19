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
#include 	<unistd.h>

#define MAXLINE 1024

#define SA struct sockaddr

#define LISTENQ 2


static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

void
sig_pipe(int signo)
{
	printf("Server received SIGPIPE - Default action is exit \n");
	exit(1);
}

static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

/* end readline */

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		perror("readline error");
	return(n);
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

int
main(int argc, char **argv)
{
	int			listenfd, connfd, sockfd;
	pid_t			childpid;
	socklen_t		clilen;
	struct sockaddr_in6	cliaddr, servaddr;
	void			sig_chld(int);
	int			i, maxi, maxfd, n; 
	int			nready, client[FD_SETSIZE];
	fd_set			rset, allset;
	char 			buf[MAXLINE], addr_buf[INET6_ADDRSTRLEN+1];

	signal(SIGPIPE, sig_pipe);

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

	fprintf(stderr,"Waiting for clients ... \n");
	
	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);


	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		
		if ( (nready = select(maxfd+1, &rset, NULL, NULL, NULL)) < 0){
				perror("select error");
				exit(1);
		}


		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
				perror("accept error");
				exit(1);
			}

			bzero(addr_buf, sizeof(addr_buf));
			inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  addr_buf, sizeof(addr_buf));

			printf("New client: %s, port %d\n",
					addr_buf, ntohs(cliaddr.sin6_port));

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			if (i == FD_SETSIZE){
				fprintf(stderr,"too many clients");
				continue;
			}
			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = Readline(sockfd, buf, MAXLINE)) == 0) {
						/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else
					//sleep(1); //processing delay simulation
					Writen(sockfd, buf, n);

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}


