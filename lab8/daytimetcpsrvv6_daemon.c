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
#include	<syslog.h>
#include	<unistd.h>

#define MAXLINE 1024

//#define SA struct sockaddr

#define LISTENQ 2
//--------------------


#define	MAXFD	64


int
daemon_init(const char *pname, int facility, uid_t uid, int socket)
{
	int		i, p;
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* parent terminates */

	/* child 1 continues... */

	if (setsid() < 0)			/* become session leader */
		return (-1);

	signal(SIGHUP, SIG_IGN);
	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	chdir("/tmp");				/* change working directory  or chroot()*/
//	chroot("/tmp");

	/* close off file descriptors */
	for (i = 0; i < MAXFD; i++){
		if(socket != i )
			close(i);
	}

	/* redirect stdin, stdout, and stderr to /dev/null */
	p= open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	openlog(pname, LOG_PID, facility);
	
        syslog(LOG_ERR," STDIN =   %i\n", p);
	setuid(uid); /* change user */
	
	return (0);				/* success */
}
//----------------------

int
main(int argc, char **argv)
{
	int				listenfd, connfd;
	socklen_t			len;
	char				buff[MAXLINE], str[INET6_ADDRSTRLEN+1];
	time_t				ticks;
	struct sockaddr_in6	servaddr, cliaddr;

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
	servaddr.sin6_port   = htons(13);	/* daytime server */

        if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
                fprintf(stderr,"bind error : %s\n", strerror(errno));
                return 1;
        }

        if ( listen(listenfd, LISTENQ) < 0){
                fprintf(stderr,"listen error : %s\n", strerror(errno));
                return 1;
        }
	
	daemon_init(argv[0], LOG_USER, 1000, listenfd);
	syslog (LOG_NOTICE, "Program started by User %d", getuid ());
	syslog (LOG_INFO,"Waiting for clients ... ");
	
	for ( ; ; ) {
		len = sizeof(cliaddr);
        	if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
                	syslog(LOG_ERR,"accept error : %s\n", strerror(errno));
                	continue;
        	}

		bzero(str, sizeof(str));
	   	inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  str, sizeof(str));
		syslog (LOG_INFO,"Connection from %s\n", str);

        	ticks = time(NULL);
        	snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        	if( write(connfd, buff, strlen(buff))< 0 )
                	syslog(LOG_ERR,"write error : %s\n", strerror(errno));
		close(connfd);
	}
}
