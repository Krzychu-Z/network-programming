#include "tcp_helper.h"

int tcp_accept( int listenfd){
	int connfd;
	socklen_t			clilen;
	struct sockaddr_in6	cliaddr;
	if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
		if (errno == EINTR)
			return -1; 		/* back to for() */
		else
			perror("accept error");
			exit(1);
	}
	return connfd;
}

int
tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
	int				listenfd, n;
	const int		on = 1;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		fprintf(stderr,"tcp_listen error for %s, %s: %s\n",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;		/* error, try next one */

		if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
			perror("SO_REUSEADDR error");
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(listenfd);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL){	/* errno from final socket() or bind() */
		fprintf(stderr,"tcp_listen error for %s, %s", host, serv);
		exit(1);
	}

   if ( listen(listenfd, LISTENQ) < 0){
           fprintf(stderr,"listen error : %s\n", strerror(errno));
           return 1;
   }
	if (addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

	return(listenfd);
}

int
tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0){
		fprintf(stderr,"tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
		exit(1);
	}
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL){	/* errno set from final connect() */
		fprintf(stderr,"tcp_connect error for %s, %s\n", host, serv);
		exit(1);
	}

	freeaddrinfo(ressave);

	return(sockfd);
}

int _signal(int signo, void *func )
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
	return sigaction(signo, &act, &oact);
}
/* end signal */

void Signal(int signo, void *func)	/* for our signal() function */
{
	if ( _signal(signo, func) != 0 )
		perror("signal error");
}

/* end tcp_connect */


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
 

