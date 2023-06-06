#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/sctp.h>

#define SA      struct sockaddr
#define LISTENQ 2
#define SERV_PORT 7
#define BUFFSIZE 2048


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
	int sock_fd,msg_flags=0;
	char readbuf[BUFFSIZE];
	struct sockaddr_in servaddr, cliaddr;
	struct sctp_sndrcvinfo sri;
	struct sctp_event_subscribe evnts;
	int stream_increment=0;
	socklen_t len;
	size_t rd_sz;

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

//fprintf(stderr,"Socket = %d : %s\n", (int) sock_fd, strerror(errno));

	for ( ; ; ) {
		
		len = sizeof(struct sockaddr);
		
		if( (rd_sz = sctp_recvmsg(sock_fd, readbuf, sizeof(readbuf),
			     (SA *)&claddr, &len,
			     &sri,&msg_flags)) == -1) {
        	fprintf(stderr,"sctp_recvmsg error : %s\n", strerror(errno));
        	return -1;
        }

//fprintf(stderr,"sctp_recvmsg OK: rd_sz= %d : %s\n", (int) rd_sz, strerror(errno));
		
		if(stream_increment) {
			sri.sinfo_stream++;
			if(sri.sinfo_stream >= sctp_get_no_strms(sock_fd, sri.sinfo_assoc_id)) 
				sri.sinfo_stream = 0;
		}

		if( (sctp_sendmsg(sock_fd, readbuf, rd_sz, 
			     (SA *)&claddr, len,
			     sri.sinfo_ppid,
			     sri.sinfo_flags,
			     sri.sinfo_stream,
			     0, 0)) < 0 ){
        	    fprintf(stderr,"sctp_sendmsg : %s\n", strerror(errno));
        	    return 1;
        }
	}
}
