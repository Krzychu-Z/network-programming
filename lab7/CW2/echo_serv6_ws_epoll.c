#include    <sys/types.h>   /* basic system data types */
#include    <sys/socket.h>  /* basic socket definitions */
#include    <sys/time.h>    /* timeval{} for select() */
#include    <time.h>                /* timespec{} for pselect() */
#include    <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <errno.h>
#include    <netdb.h>
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <limits.h>		/* for OPEN_MAX */
#include    <sys/epoll.h>
#include    <unistd.h>

#define STATS
#ifdef STATS
#include 	<pthread.h>
#endif
 
#define MAXLINE 1024
#define SA struct sockaddr
#define LISTENQ 2
#define INFTIM -1
#define MAXEVENTS 2000


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


int evn_r;
int activeconns_r;
long s_r;

	
#ifdef STATS
void *
print_r(void *arg)
{
	while (1){
		fprintf(stderr,"\rService of %6d events in %10ld us : active connections: %6d", evn_r,s_r, activeconns_r);
		sleep(1);
	}
	return(NULL);
}
#endif

int
main(int argc, char **argv)
{
	int debug = 0;
	int		listenfd, listenfd_udp, connfd, sockfd;
	pid_t		childpid;
	socklen_t	clilen;
	struct sockaddr_in6	cliaddr, servaddr, servaddr_udp;
	int		i, maxi, maxfd, n; 
	int		nready, s;
	char 		buf[MAXLINE], addr_buf[INET6_ADDRSTRLEN+1];
	struct 		timeval start, stop;
	int activeconns = 0;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	
	printf("MAX EVENTS = %d\n",MAXEVENTS-2);
	
	if( argc > 1 )
		debug = atoi(argv[1]);
	
	int epollfd, eventstriggered, currfd;
	struct epoll_event events[MAXEVENTS];
	struct epoll_event ev;
	
	
		/* Set up our epoll queue */
	if((epollfd = epoll_create(MAXEVENTS)) == -1){ 	
          fprintf(stderr,"epoll_create() error : %s\n", strerror(errno));
          return -1;
   }
	
//Create listening sockets	
	if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"TCP socket error : %s\n", strerror(errno));
		return -1;
	}
	int on = 1;               
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
	      fprintf(stderr,"SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
	}

	if ( (listenfd_udp = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"UDP socket error : %s\n", strerror(errno));
		return 1;
	}   

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(7);	/* echo server */

	if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"TCP bind error : %s\n", strerror(errno));
		return 1;
	}

	if ( bind( listenfd_udp, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"UDP bind error : %s\n", strerror(errno));
		return 1;
	}

	if ( listen(listenfd, LISTENQ) < 0){
		fprintf(stderr,"listen error : %s\n", strerror(errno));
		return 1;
	}

	activeconns +=2;	
	activeconns_r = activeconns;
	int time_start=0;
	int evn;

	ev.events = EPOLLIN; //Specify which event to listen for
	ev.data.fd = listenfd_udp; // Specify user data.
	
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd_udp, &ev) == -1){
           fprintf(stderr,"listen error : %s\n", strerror(errno));
           return 1;
	}

	ev.events = EPOLLIN; //Specify which event to listen for
	ev.data.fd = listenfd; // Specify user data.

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
           fprintf(stderr,"listen error : %s\n", strerror(errno));
           return 1;
	}
	

#ifdef STATS
	int ret;
	pthread_t	tid;
	if ( (ret=pthread_create(&tid, NULL, print_r, NULL)) != 0 ) {
	    errno=ret;
	    fprintf(stderr, "pthred_create() error : %s", strerror(ret));
	}
#endif

#define AVG 1000
	long ss[AVG];
	int count=0;
	int iter=0;
	
	for ( ; ; ) {


	if( gettimeofday(&stop,0) != 0 ){
		fprintf(stderr,"gettimeofday error : %s\n", strerror(errno));
	}else{
		if(time_start == 1){
			long s =(long)((stop.tv_sec*1000000 + stop.tv_usec) - (start.tv_sec*1000000 + start.tv_usec));
			if(count < 100) count++;
			if(iter > 99) iter=0;
			ss[iter]=s;
			long sum=0;
			int k;
			for(k=0; k < count; k++) sum+=ss[iter];	
			iter++; 
			s_r=sum/count; 	evn_r = evn; 	activeconns_r =  activeconns;
			time_start=0;  
		}
	}
	
    	if( (nready = epoll_wait(epollfd, events, MAXEVENTS, -1)) == -1){
           fprintf(stderr,"epoll_wait error : %s\n", strerror(errno));
           return 1;
		}
		
		if(debug)
			printf ("New events=%d, active connections: %d\n", nready, activeconns);
			

	if( gettimeofday(&start,0) != 0 ){
	      fprintf(stderr,"gettimeofday error : %s\n", strerror(errno));
	}else{
	      time_start=1;
	      evn = nready;
	}

	for (i = 0; i < nready; i++) {	/* check all clients for data */

		      
      //TCP listen socket
	      if (events[i].data.fd == listenfd) { //e
			clilen = sizeof(cliaddr);
			if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
				    //  perror("accept error");
				      continue;
			}

      if(debug)		   
      printf ("\tnew TCP client: events=%d, sockfd = %d, on socket = %d,  activeconns = %d \n", nready, listenfd, connfd, activeconns);
		      
			bzero(addr_buf, sizeof(addr_buf));
			inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  addr_buf, sizeof(addr_buf));
			if(debug)
			      printf("\tNew client: %s, port %d\n",	addr_buf, ntohs(cliaddr.sin6_port));

//			if ((activeconns+1) >= MAXEVENTS) { //epoll
//			      // Too many connections for this program
//				      fprintf(stderr, "accept() error - too many connections for this program\n");
//				      close(connfd);
//				      continue;
//			}
			ev.events = EPOLLIN ; // available for input and non edge_triggered
			ev.data.fd = connfd;
			if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev)){
				      perror("epoll_ctl adding new connection error");
				      close(connfd);
				      continue;
			}
			activeconns ++;
			continue;
		  } //end of epoll
      //end of TCP listen socket

      //UDP clients
	      if (events[i].data.fd == listenfd_udp) { //epoll
if(debug) printf ("UDP connection\n");
			int nread;
			peer_addr_len = sizeof(struct sockaddr_storage);
			nread = recvfrom(listenfd_udp, buf, MAXLINE, 0,
			(struct sockaddr *) &peer_addr, &peer_addr_len);
			if (nread == -1)
			      continue;      /* Ignore failed request */

			char host[NI_MAXHOST], service[NI_MAXSERV];

			s = getnameinfo((struct sockaddr *) &peer_addr,
			peer_addr_len, host, NI_MAXHOST,
			service, NI_MAXSERV, NI_NUMERICHOST);
			if (s == 0)
			      printf("UDP: Received %ld bytes from %s:%s\n",
				(long) nread, host, service);
			else
			      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

			if (sendto(listenfd_udp, buf, nread, 0, (struct sockaddr *) &peer_addr,
									peer_addr_len) != nread)
			      fprintf(stderr, "UDP Error sending response\n");

			continue; 
		}
      //end of udp clients

      //tcp clients
		currfd = events[i].data.fd;
if(debug)		printf ("\tRead client\n: events=%d, sockfd = %d\n", nready, currfd);
		if((n = read(currfd, buf, sizeof buf)) == -1) {
		      // Closing the descriptor will make epoll remove it from the set of descriptors which are monitored.
		      perror("read error - closing connection");
		      close(currfd);
		      activeconns --;
		      continue;
		}
		      // We successfully read from the socket. Check if there is data
		if(n == 0) {
		      // The socket sent EOF.
		      // Closing the descriptor will make epoll remove it from the set of descriptors which are monitored.
		      close (currfd);
		      activeconns --;
		      continue;
		}
		if( write(currfd, buf, n) == -1) {
		      // Something went wrong.
		      close(currfd);
		      activeconns --;
		      continue;
		}
      //epoll end
	    }
	}
}
