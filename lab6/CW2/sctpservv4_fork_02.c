#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/sctp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define SA      struct sockaddr
#define LISTENQ 2
#define SERV_PORT 7
#define BUFFSIZE 2048
#define MAXLINE 2048


void
sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

void
sig_pipe(int signo)
{
	printf("Server received SIGPIPE - Default action is exit \n");
	exit(1);
}

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

void
str_echo(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];

again:
	while ( (n = read(sockfd, buf, MAXLINE)) > 0){
//		sleep(1); //Sztuczne opóŸnienie dla wymuszenia sytuacji awaryjnych
		Writen(sockfd, buf, n);
	}
	if (n < 0 && errno == EINTR)
		goto again;
	else if (n < 0)
		perror("str_echo: read error");
}


int 
sctp_get_no_strms(int sock_fd, sctp_assoc_t assoc_id)
{
	int retsz;
	struct sctp_status status;
	retsz = sizeof(status);	
	bzero(&status,sizeof(status));

	status.sstat_assoc_id = assoc_id;
	if( sctp_opt_info(sock_fd,assoc_id, SCTP_STATUS,
		   &status, &retsz) <0 ){
		perror("sctp_get_no_strms:sctp_opt_info error");
		exit(-1);
	}

	return(status.sstat_outstrms);
}

int
main(int argc, char **argv)
{
	int sock_fd,msg_flags;
	char readbuf[BUFFSIZE];
	struct sockaddr_in servaddr, cliaddr;
	struct sctp_sndrcvinfo sri;
	struct sctp_event_subscribe evnts;
	int stream_increment=0;
	socklen_t len;
	size_t rd_sz;
	int connfd, childpid;

    struct sockaddr claddr; 

	if (argc == 2)
		stream_increment = atoi(argv[1]);
    if ( (sock_fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	if( bind(sock_fd, (SA *) &servaddr, sizeof(servaddr)) == -1 ){
        fprintf(stderr,"bind error : %s\n", strerror(errno));
        return 1;
    }
	
	
	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event = 1;
	if(setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
		   &evnts, sizeof(evnts)) == -1 ){
        fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
        return 1;
    }

	if ( listen(sock_fd, LISTENQ) == -1){
        fprintf(stderr,"listen error : %s\n", strerror(errno));
        return 1;
    }

	signal(SIGCHLD, sig_chld);
	signal(SIGPIPE, sig_pipe);

	/* include mod_servfork */
	for ( ; ; ) {
		len = sizeof(struct sockaddr_in);
		rd_sz = sctp_recvmsg(sock_fd, readbuf, sizeof(readbuf),
			     (SA *)&cliaddr, &len,
			     &sri,&msg_flags);
		if( rd_sz < 0 ){
			perror("sctp_recvmsg error");
			continue;
		}		 

		rd_sz = sctp_sendmsg(sock_fd, readbuf, rd_sz, 
			     (SA *)&cliaddr, len,
			     sri.sinfo_ppid,
			     sri.sinfo_flags,
			     sri.sinfo_stream,
			     0, 0);

		if( rd_sz < 0 ){
			perror("sctp_sendmsg error");
			/*close association - to be done*/
		}		 

		connfd = sctp_peeloff(sock_fd,sri.sinfo_assoc_id);
		if(connfd == -1){
			perror("sctp_peeloff fails");
			continue;
		}
		if((childpid = fork()) == 0) {
			close(sock_fd);
			str_echo(connfd);
			exit(0);
		} else {
			close(connfd);
		}
	}
/* end mod_servfork */
	
}
