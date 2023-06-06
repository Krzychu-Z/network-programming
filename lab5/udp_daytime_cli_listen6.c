#include 	<sys/types.h>
#include 	<sys/socket.h>
#include 	<netdb.h>
#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<unistd.h>
#include 	<string.h>
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

#define MAXLINE 500

int
main(int argc, char *argv[])
{

	struct timeval delay;
	int	sfd, n, s, i;
	socklen_t			len;
	char				recvline[MAXLINE], str[INET6_ADDRSTRLEN+1];
	time_t				ticks;
	struct sockaddr_in6	servaddr, peer_addr;
	char host[NI_MAXHOST], service[NI_MAXSERV];

   if ( (sfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
          fprintf(stderr,"socket error : %s\n", strerror(errno));
          return 1;
   }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(13);	/* daytime server */

	if ( bind( sfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"bind error : %s\n", strerror(errno));
		return 1;
	}

	delay.tv_sec =6;  //opoznienie na gniezdzie
	delay.tv_usec = 0; 
	len = sizeof(delay);
	if( setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
		fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
		return 1;
	}		
		
	for( i=0 ; i < 3 ; i++){
	        /* Read data from server */
		fprintf(stderr, "Waiting for server ...\n");
	
		len = sizeof(peer_addr);	
		if( (n = recvfrom(sfd, recvline, MAXLINE, 0, (struct sockaddr *) &peer_addr, 
				&len) ) < 0 ){ 
	
			perror("recfrom error");
			if( errno == (EAGAIN | EWOULDBLOCK)) {
				printf("Timeout - no serwers\n");
				exit(1);
			}
			else{
				printf("RECVFROM error\n");
				exit(1);
			}
		}
	
		s = getnameinfo((struct sockaddr *) &peer_addr,
						len, host, NI_MAXHOST,
						service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
		if (s == 0)
			printf("Received %ld bytes from %s:%s\n",
								(long) n, host, service);
		else
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
			
		recvline[n] = 0;	/* null terminate */
		if (fputs(recvline, stdout) == EOF){
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			exit(1);
		}        
			
	}
	exit(EXIT_SUCCESS);
}
