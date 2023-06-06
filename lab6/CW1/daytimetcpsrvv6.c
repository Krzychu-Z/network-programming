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
#include	<unistd.h>

#define MAXLINE 1024

//#define SA struct sockaddr

#define LISTENQ 2
#define SLEEP 1

int
main(int argc, char **argv)
{
	int				listenfd, connfd;
	socklen_t			len;
	char				buff[MAXLINE], str[INET6_ADDRSTRLEN+1];
	time_t				ticks;
	struct sockaddr_in6	servaddr, cliaddr;

        if ( (listenfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
        }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(13);	/* daytime server */

        if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
                fprintf(stderr,"bind error : %s\n", strerror(errno));
                return 1;
        }

        if ( listen(listenfd, LISTENQ) < 0){
                fprintf(stderr,"listen error : %s\n", strerror(errno));
                return 1;
        }
	fprintf(stderr,"Waiting for clients ... \n");
	for ( ; ; ) {
		len = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}
		if(SLEEP)
			sleep(2);
		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);

        	ticks = time(NULL);
        	snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        	if( write(connfd, buff, strlen(buff))< 0 )
                	fprintf(stderr,"write error : %s\n", strerror(errno));
		shutdown(connfd, SHUT_RD);
		if(SLEEP)
			sleep(10);
		close(connfd);
	}
}
