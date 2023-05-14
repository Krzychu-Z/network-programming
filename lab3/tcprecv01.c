#include "tcp_helper.h"
#include <sys/ioctl.h>
#include <fcntl.h>               /* for nonblocking */

void	sig_urg(int);

int		connfd;

int main(int argc, char **argv)
{
	int n;
	char buff[100];
	int listenfd;
	int pid;
	
	if (argc == 2)
		listenfd = tcp_listen(NULL, argv[1], NULL);
	else if (argc == 3)
		listenfd = tcp_listen(argv[1], argv[2], NULL);
	else{
		fprintf(stderr,"usage: tcprecv01 [ <host> ] <port#>\n");
		return 1;
	}

	connfd = accept(listenfd, NULL, NULL);

	Signal(SIGURG, sig_urg);
	if(fcntl(connfd, F_SETOWN, getpid())== -1);
		perror("fcntl:");
	pid = getpid();
	if( ioctl(connfd, SIOCSPGRP, &pid) != 0)
		perror("ioctl:");

	for ( ; ; ) {
		if ( (n = read(connfd, buff, sizeof(buff)-1)) == 0) {
			printf("received EOF\n");
			exit(0);
		}
		buff[n] = 0;	/* null terminate */
		printf("read %d bytes: %s\n", n, buff);
	}
	
	
}

void
sig_urg(int signo)
{
	int		n;
	char	buff[100];

	printf("SIGURG received\n");
	n = recv(connfd, buff, sizeof(buff)-1, MSG_OOB);
	if( n == -1 ){
	   perror("OOB read error");
	   return;
	}
	buff[n] = 0;		/* null terminate */
	printf("read %d OOB byte: %s\n", n, buff);
}
