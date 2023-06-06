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
#define SA struct sockaddr

void
day_time_br(int sockfd, SA *pcliaddr, socklen_t clilen)
{
	int		n;
	socklen_t	len = clilen;
	char		mesg[MAXLINE];
	char		str[INET6_ADDRSTRLEN+1];
	time_t		ticks;

	fprintf(stderr,"Sending day time for clients ... \n");

	for ( ; ; ) {
		ticks = time(NULL);
		snprintf(mesg, sizeof(mesg), "PS daytime server: %.24s\r\n", ctime(&ticks));
		fprintf(stdout,"Sending day time for clients : %s  \n", mesg);
		
		if( sendto(sockfd, mesg, strlen(mesg), 0, pcliaddr, len) < 0 ) {
               	fprintf(stderr,"sendto error : %s\n", strerror(errno));
             	continue;
		}
		sleep(2);
	}
}

	

int
main(int argc, char **argv)
{
	int					sockfd, on;
	int					len;
	char				buff[MAXLINE], str[INET6_ADDRSTRLEN+1];
	time_t				ticks;
	struct sockaddr_in	servaddr;  /*Service address*/
	struct servent		*sp;

	if (argc != 2){
		fprintf(stderr, "usage: %s <IPv4 address> \n",argv[0]);
		return 1;
	}
		
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13); /* PS daytime service */
	
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}
	
	len = sizeof(on);
	on=1;
	if( setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, len) == -1 ){
			fprintf(stderr,"SO_BROADCAST setsockopt error : %s\n", strerror(errno));
			return 1;
	}		

	day_time_br(sockfd, (SA *) &servaddr, sizeof(servaddr));
        
	return 0;
}
