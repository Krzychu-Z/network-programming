#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/sctp.h>
#include <unistd.h>

#define SA struct sockaddr
#define MAXLINE 128
#define SERV_PORT 7
#define	SCTP_MAXLINE	800
#define SERV_MAX_SCTP_STRM	10	/* normal maximum streams */
#define SERV_MORE_STRMS_SCTP	20	/* larger number of streams */


int
sctpstr_cli(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
	struct sockaddr_in peeraddr;
	struct sctp_sndrcvinfo sri;
	char sendline[SCTP_MAXLINE], recvline[MAXLINE];
	socklen_t len;
	int out_sz,rd_sz;
	int msg_flags=0;

	bzero(&sri,sizeof(sri));
	printf("Enter text in format:'[streamnum]text'\n");
	while (fgets(sendline, SCTP_MAXLINE, fp) != NULL) {
		if(sendline[0] != '[') {
			printf("Error, line must be of the form '[streamnum]text'\n");
			continue;
		}
		sri.sinfo_stream = strtol(&sendline[1],NULL,0);
		out_sz = strlen(sendline);
		if ( sctp_sendmsg(sock_fd, sendline, out_sz, 
			     to, tolen, 
			     0, 0,
			     sri.sinfo_stream,
			     0, 0) == -1 ){
		    fprintf(stderr,"sctp_sendmsg error : %s\n", strerror(errno));
		    return 1;
	    }

		len = sizeof(peeraddr);
		if( (rd_sz = sctp_recvmsg(sock_fd, recvline, sizeof(recvline),
			     (SA *)&peeraddr, &len,
			     &sri,&msg_flags)) == -1 ){
		    fprintf(stderr,"sctp_recvmsg error : %s\n", strerror(errno));
		    return 1;
	    }

//		if( msg_flags & MSG_EOR )
//			printf("End of message\n");

		printf("From str:%d seq:%d (assoc:0x%x):",
		       sri.sinfo_stream,sri.sinfo_ssn,
		       (u_int)sri.sinfo_assoc_id);
		printf("%.*s",rd_sz,recvline);
	}
} 





int
main(int argc, char **argv)
{
	int sock_fd;
	struct sockaddr_in servaddr;
	struct sctp_event_subscribe evnts;

	if(argc < 2){
		fprintf(stderr, "Missing host argument - use '%s host [echo]'\n",
		       argv[0]);
		return 1;
	}
	

    if ( (sock_fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 1 ){
		fprintf(stderr,"inet_pton error \n");
		return 1;
	}

	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event = 1;

	if( setsockopt(sock_fd,IPPROTO_SCTP, SCTP_EVENTS,
		   &evnts, sizeof(evnts)) > 0 ){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	
	sctpstr_cli(stdin,sock_fd,(SA *)&servaddr,sizeof(servaddr));
	
	return(0);
}
