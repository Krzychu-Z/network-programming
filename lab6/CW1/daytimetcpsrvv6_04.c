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
#include	<unistd.h>

#define MAXLINE 1024
#define COUNT 10000
#define PORT_NR 13

//#define SA struct sockaddr

#define LISTENQ 2

int
main(int argc, char **argv)
{
	int  listenfd, connfd, sndbuf;
	socklen_t  slen;
	char  buff[MAXLINE], str[INET6_ADDRSTRLEN+1];
	time_t  ticks;
	struct sockaddr_in6  servaddr, cliaddr;
    int  mss,i,len;
    int bufsize=2000;
    struct timeval start, stop;


        if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
                fprintf(stderr,"socket error : %s\n", strerror(errno));
                return 1;
        }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	if(argc == 1)
		servaddr.sin6_addr   = in6addr_any;
	else{
		if( inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) != 1 ){
			printf("ERROR: Address format error\n");
			return -1;
		}
	}
	servaddr.sin6_port   = htons(PORT_NR);	/* daytime server */

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


        len = sizeof(sndbuf);
        if (getsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len) < 0){
                fprintf(stderr,"SO_SNDBUF getsockopt error : %s\n", strerror(errno));
        }

        len = sizeof(mss);
        if (getsockopt(listenfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) < 0){
                fprintf(stderr,"TCP_MAXSEG getsockopt error : %s\n", strerror(errno));
        }

        printf("defaults: SO_SNDBUF = %d, MSS = %d\n", sndbuf, mss);

#ifdef SND_BUF
        sndbuf = bufsize;               /* a prime number */
        if (setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0){
                fprintf(stderr,"SO_SNDBUF setsockopt error : %s\n", strerror(errno));
        }
        
        len = sizeof(sndbuf);
        if (getsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len) < 0){
                fprintf(stderr,"SO_SNDBUF getsockopt error : %s\n", strerror(errno));
        }

        len = sizeof(mss);
        if (getsockopt(listenfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) < 0){
                fprintf(stderr,"TCP_MAXSEG getsockopt error : %s\n", strerror(errno));
        }

        printf("SO_SNDBUF = %d (after setting it to %i), MSS = %d\n", sndbuf, bufsize, mss);

#define MSS 1300
        mss=MSS;
        if( setsockopt(listenfd, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)) == -1){
                fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
                return 1;
        }
        len = sizeof(mss);
        if( getsockopt(listenfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) == -1 ){
                fprintf(stderr,"getsockopt error : %s\n", strerror(errno));
                return 3;
        }
        printf("MSS: = %d (after setting it to %d)\n", mss, MSS);

#endif //SND_BUF

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

                len = sizeof(sndbuf);
                if (getsockopt(connfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len) < 0){
                        fprintf(stderr,"SO_SNDBUF getsockopt error : %s\n", strerror(errno));
                }

                len = sizeof(mss);
                if (getsockopt(connfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len) < 0){
                        fprintf(stderr,"TCP_MAXSEG getsockopt error : %s\n", strerror(errno));
                }

                printf("SO_SNDBUF = %d (after CONNECT), MSS = %d\n", sndbuf, mss);
                

		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);

        for(i=0; i < MAXLINE; i++){
            buff[i]='z';
        }


     	ticks = time(NULL);
       	snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        if( write(connfd, buff, strlen(buff))< 0 )
                     fprintf(stderr,"write error : %s\n", strerror(errno));
               
        gettimeofday(&start,0);
                //for(i=0; i < COUNT; i++){
        i=1;
        while(1){
            i++;   
            if( write(connfd, buff, MAXLINE)< 0 )
              	fprintf(stderr,"write error : %s\n", strerror(errno));
            gettimeofday(&stop,0);
            if( (stop.tv_sec - start.tv_sec) > 5 )
                break;
        }
        printf("Wrote  %d segments\n", i);
        fflush(stdout);
                   
		close(connfd);
	}
}
