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
int licznik=0;
const SA *pservaddr;
int	sockfd, len;

void sig_alarm(int signo)
{
   char sendline[24];
   printf("Received SIGALARM = %d\n", signo);
   if( licznik > 2 ){
      fprintf(stderr,"Koncze program : brak odpowiedzi\n");
      exit(1);
   }
   if( sendto(sockfd, 0, 0, 0, pservaddr, len) <0 ){
   	perror("sendto error");
   	exit(1);
   }
   licznik++;
   alarm(3);
}

int m_signal(int signum, void handler(int)){
    struct sigaction new_action, old_action;

  /* Set up the structure to specify the new action. */
    new_action.sa_handler = handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    if( sigaction (signum, &new_action, &old_action) < 0 ){
          fprintf(stderr,"sigaction error : %s\n", strerror(errno));
          return 1;
    }
    return 0;
}

int
dt_cli(int sockfd, const SA *pservaddr, socklen_t servlen)
{
	int		n, i=0;
	char		sendline[MAXLINE], recvline[MAXLINE + 1];
//	socklen_t	len;
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

	m_signal(SIGALRM, sig_alarm);

	len = servlen;

	alarm(1);

	if( (n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr, &len) ) < 0 ){
		printf("errno = %d\n", errno);
		perror("recfrom error");
		free(preply_addr);
		return(1);
	}else
	        alarm(0);

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
	int	n, delay;
	struct sockaddr_in6	servaddr;
	char				recvline[MAXLINE + 1];

	pservaddr = (SA *) &servaddr;
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
