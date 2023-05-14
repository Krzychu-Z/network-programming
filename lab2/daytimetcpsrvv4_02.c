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
#include        <netinet/tcp.h>         /* for TCP_MAXSEG */
#include        <unistd.h>


#define MAXLINE 10240
#define COUNT 10000
#define PORT_NR 13

//#define SA struct sockaddr

#define LISTENQ 2

int
main(int argc, char **argv)
{
	int  listenfd, connfd, sndbuf;
	socklen_t  slen;
	char  buff[MAXLINE], str[INET_ADDRSTRLEN+1];
	time_t  ticks;
	struct sockaddr_in  servaddr, cliaddr;
        int  mss,i,len;
        int bufsize=2000;
        struct timeval start, stop;
	struct tcp_info tcp_i;


        if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
        }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(argc == 1)
		servaddr.sin_addr.s_addr   = htonl(INADDR_ANY);
	else{
		if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1 ){
			printf("ERROR: Address format error\n");
			return -1;
		}
	}
	servaddr.sin_port   = htons(PORT_NR);	/* daytime server */

#define REUSEADDR
#ifdef REUSEADDR	
        sndbuf = 1;               
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &sndbuf, sizeof(sndbuf)) < 0){
                fprintf(stderr,"SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
        }
#endif



    if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"bind error : %s\n", strerror(errno));
        return 1;
    }


    if ( listen(listenfd, LISTENQ) < 0){
        fprintf(stderr,"listen error : %s\n", strerror(errno));
        return 1;
    }


	for ( ; ; ) {
		slen = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &slen)) < 0){
                	fprintf(stderr,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);
 
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
            if( write(connfd, buff, strlen(buff))< 0 )
                fprintf(stderr,"write error : %s\n", strerror(errno));
                      
	        bzero(&tcp_i, sizeof(tcp_i));               
		int size = sizeof(tcp_i);
	        if (getsockopt(connfd, SOL_TCP, TCP_INFO, &tcp_i, &size) < 0){
	                fprintf(stderr,"TCP_INFO setsockopt error : %s\n", strerror(errno));
	        }
		else{
			printf("TCP info: SND_MSS=%u : PMTU=%u \n", (unsigned)tcp_i.tcpi_snd_mss, (uint)tcp_i.tcpi_pmtu );
		}
		close(connfd);
	}
}
