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
#include        <unistd.h>


#define MAXLINE 1024
#define SA      struct sockaddr

int
dt_cli(int sockfd, const SA *pservaddr, socklen_t servlen)
{
	int		n, i;
	char		sendline[MAXLINE], recvline[MAXLINE + 1];
	socklen_t	len;
	struct sockaddr	*preply_addr;
	char		str[INET6_ADDRSTRLEN+1];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	struct timeval delay;

	if( (preply_addr = malloc(servlen)) == NULL ){
		perror("malloc error");
		exit(1);
	}

	bzero( sendline, sizeof(sendline));

	delay.tv_sec = 3;  //opoznienie na gniezdzie
	delay.tv_usec = 1; 
	len = sizeof(delay);
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
		fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
		return -1;
	}

	len = servlen;
	for(i=0; i < 3 ; i++ ){

		if( sendto(sockfd, sendline, 0, 0, pservaddr, servlen) <0 ){
			perror("sendto error");
			free(preply_addr);
			exit(1);
		}

		if( (n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr, &len) ) < 0 ){
			printf("errno = %d\n", errno);
			perror("recvfrom error");
			if( (errno == EAGAIN) && (i < 2) )
				continue;
			else{
				free(preply_addr);
				return(1);
			}
		}else
			break;
	}

	bzero(str, sizeof(str));

	if( preply_addr->sa_family == AF_INET6 ){
		cliaddr = (struct sockaddr_in6*) preply_addr;
		inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  str, sizeof(str));
	}
	else{
		cliaddrv4 = (struct sockaddr_in*) preply_addr;
		inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  str, sizeof(str));
	}

	printf("Time from %s (%d)\n", str,n);

	if (len != servlen || memcmp(pservaddr, preply_addr, len) != 0) {
		printf("reply from %s (ignored)\n", str);
	}

	recvline[n] = 0;	/* null terminate */
	if (fputs(recvline, stdout) == EOF){
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		free(preply_addr);
		exit(1);
	}
	free(preply_addr);
	return 0;
}

	void
dt_cli_connect(int sockfd, const SA *pservaddr, socklen_t servlen)
{
	int		n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];
	char		str[INET6_ADDRSTRLEN+1];

	bzero(str, sizeof(str));
	if( connect(sockfd, (SA *) pservaddr, servlen) < 0 ){
		perror("connect error");
		exit(1);
	}

	if( write(sockfd, sendline, strlen(sendline)+1) < 0 ){
		perror("write error");
		exit(1);
	}

	if( (n = read(sockfd, recvline, MAXLINE)) < 0 ){
		perror("read error");
		exit(1);
	}

	recvline[n] = 0;	/* null terminate */
	if (fputs(recvline, stdout) == EOF){
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		exit(1);
	}
}

	int
main(int argc, char **argv)
{
	int					sockfd, n, delay;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];

	if (argc != 2){
		fprintf(stderr, "usage: a.out <IPaddress> \n");
		return 1;
	}
	if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port   = htons(13);	/* daytime server */
	if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0){
		fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}

	dt_cli( sockfd, (SA *) &servaddr, sizeof(servaddr));
	//	dt_cli_connect( sockfd, (SA *) &servaddr, sizeof(servaddr));

	printf("W ciagu dziesieciu sekund mozna podgladac stan otwartego gniazda: netstat -u6ap \n\n");
	fflush(stdout);
	sleep(10);	
	exit(0);
}
