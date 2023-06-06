#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>


#define MAXLINE 500

int main(int argc, char *argv[])
{
	int	sfd, n, on=1, olen, i;
	struct sockaddr_in	servaddrv4, peer_addr;
	int s, peer_addr_len, addr_len, recv_flag=0;
	char	recvline[MAXLINE + 1];
	struct timeval delay;
	char host[NI_MAXHOST], service[NI_MAXSERV];  

	if (argc != 2){
		fprintf(stderr, "usage: %s <IPaddress> \n", argv[0]);
		return 1;
	}

	bzero(&servaddrv4, sizeof(servaddrv4));

	if (inet_pton(AF_INET, argv[1], &servaddrv4.sin_addr) <= 0){
		fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}else{
		servaddrv4.sin_family = AF_INET;
		servaddrv4.sin_port   = htons(13);	/* daytime server */
		addr_len =  sizeof(servaddrv4);
		if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
			return 2;
		}

	}

	/*set up options */
	olen = sizeof(on);
	on=1;
	if( setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &on, olen) == -1 ){
		fprintf(stderr,"SO_BROADCAST setsockopt error : %s\n", strerror(errno));
		return 3;
	}				  

 //       setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, "wlp9s0", 4);
			
	delay.tv_sec = 3;  //opoznienie na gniezdzie
	delay.tv_usec = 0; 
	olen = sizeof(delay);
	if( setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &delay, olen) == -1 ){
				fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
			return 4;
	}

    /* Send request - empty datagram to server  and receive responses*/

	for(i=0; i < 4 ; i++ ){

		if(recv_flag == 0 )
			if( sendto(sfd, recvline, 0, 0, (struct sockaddr *)&servaddrv4 , addr_len) <0 ){
				perror("sendto error");
				exit(1);
			}
		peer_addr_len = sizeof(peer_addr);
		if( (n = recvfrom(sfd, recvline, MAXLINE, 0, (struct sockaddr *) &peer_addr, 
																	&peer_addr_len )) < 0 ){
			if( (errno == (EAGAIN | EWOULDBLOCK)) && (i < 2) ){
				printf("Waiting for response ...\n");
				continue;
			}
			else{
				perror("recvfrom error");
				exit(1);
			}
		}else{
			recv_flag=1;
			break;
		}
	}
			
	s = getnameinfo((struct sockaddr *) &peer_addr,
				peer_addr_len, host, NI_MAXHOST,
				service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
	if (s == 0)
		printf("Received %ld bytes from %s:%s\n", (long) n, host, service);
	else
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
		
	recvline[n] = 0;	/* null terminate */
	if (fputs(recvline, stdout) == EOF){
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		exit(1);
	}        		
				
	exit(EXIT_SUCCESS);
}
