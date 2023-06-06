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
#include	<netinet/tcp.h>		/* for TCP_MAXSEG */
#include	<unistd.h>

#define SA      struct sockaddr
#define MAXLINE 2048

int
main(int argc, char **argv)
{
	int					sockfd, rcvbuf, mss;
	socklen_t			len;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];
	int err,n;
	struct timeval start, stop;

	if (argc != 2){
		fprintf(stderr, "ERROR: usage: %s <IPv6 address>\n", argv[0]);
		return 1;
	}

	if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
    }

	len = sizeof(rcvbuf);
	if( getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len) == -1 )
	{	
		fprintf(stderr,"getsockopt error : %s\n", strerror(errno));
		return 2;
	}
	len = sizeof(mss);
	if( getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) == -1 ){
		fprintf(stderr,"getsockopt error : %s\n", strerror(errno));
		return 3;
	}
	printf("defaults: SO_RCVBUF = %d, MSS = %d\n", rcvbuf, mss);

#ifdef OPTION_SET
#define RCVBUF 9973   /* a prime number */
	rcvbuf = RCVBUF;		
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) == -1){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}
	len = sizeof(rcvbuf);
	if( getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len) == -1){
		fprintf(stderr,"getsockopt error : %s\n", strerror(errno));
		return 4;
	}
	printf("SO_RCVBUF = %d (after setting it to %d)\n", rcvbuf, RCVBUF);

#define MSS 1300
	mss=MSS;
	if( setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) == -1){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}
	len = sizeof(mss);
	if( getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) == -1 ){
		fprintf(stderr,"getsockopt error : %s\n", strerror(errno));
		return 3;
	}
	printf("MSS: = %d (after setting it to %d)\n", mss, MSS);
	
#endif //OPTION_SET

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(13);		/* daytime server */
	int er;
	if(  (er=inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr)) == -1 ){
		fprintf(stderr,"inet_pton error : %s\n", strerror(errno));
		return 1;
	}else if(er=0)
	{ printf("Addres error \n");
		return 1;
	}
	

	if( connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) == -1){
		fprintf(stderr,"connect: %s\n", strerror(errno));
		return 1;
	}

	len = sizeof(rcvbuf);
	if( getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len)== -1) {
		fprintf(stderr,"getsockopt error (5) : %s\n", strerror(errno));
		return 5;

	}
	len = sizeof(mss);
	if( getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) == -1){
		fprintf(stderr,"getsockopt error (6) : %s\n", strerror(errno));
		return 6;
	}
	printf("After connect: SO_RCVBUF = %d, MSS = %d\n", rcvbuf, mss);

	int time_start=0;
	if( gettimeofday(&start,0) != 0 ){
		fprintf(stderr,"gettimeofday error : %s\n", strerror(errno));
	}else{
		time_start=1;
	}
	int count=0;
	unsigned long size=0; 
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	/* null terminate */
		if(count == 0 )
		  if ( fputs(recvline, stdout) == EOF) {
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			return 1;
		  }
		count++;
		size+=n;
	}
	if (n < 0)
		fprintf(stderr,"read error : %s\n", strerror(errno));
	int s;
	if( gettimeofday(&stop,0) != 0 ){
		fprintf(stderr,"gettimeofday error : %s\n", strerror(errno));
	}else{
		if(time_start == 1){
			s=(int)((stop.tv_sec*1000000 + stop.tv_usec) - (start.tv_sec*1000000 + start.tv_usec));
			double tr = (size*8.0)/(s);
		    fprintf(stderr,"Reading from socket in: %d ms, : in %d segments (%lu)(%lf mbit/s)\n", s/1000,
		                   count, size, tr );
		}
	 
	}

	fflush(stdout);

	exit(0);
}
