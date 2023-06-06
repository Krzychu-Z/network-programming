#include  <sys/types.h>   /* basic system data types */
#include  <sys/socket.h>  /* basic socket definitions */
#include  <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>   /* inet(3) functions */
#include  <errno.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <netinet/sctp.h>
#include  <unistd.h>

#define SA      struct sockaddr
#define MAXLINE 2048
#define SERV_PORT 7
#define	SCTP_MAXLINE	800
#define SERV_MAX_SCTP_STRM	10	/* normal maximum streams */
#define SERV_MORE_STRMS_SCTP	20	/* larger number of streams */


char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char		portstr[8];
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return(str);
	}
/* end sock_ntop */

	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		str[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
			return(NULL);
		if (ntohs(sin6->sin6_port) != 0) {
			snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
			strcat(str, portstr);
			return(str);
		}
		return (str + 1);
	}

	default:
		snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}



void
sctp_print_addresses(struct sockaddr *addrs, int num)
{
	struct sockaddr *ss;
	int i,salen;

	ss = addrs;
	for(i=0; i<num; i++){
		printf("%s\n", sock_ntop((SA *)ss, salen));

		switch(ss->sa_family) {
		case AF_INET:
			salen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			salen = sizeof(struct sockaddr_in6);
			break;
		default:
			fprintf(stderr,"sctp_print_addresses: unknown AF");
			break;
		}
		ss = (struct sockaddr *)((char *)ss + salen);
		
	}
}



void
check_notification(int sock_fd,char *recvline,int rd_len)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
//	struct sockaddr_storage *sal,*sar;
	struct sockaddr *sal,*sar;
	int num_rem, num_loc;

	snp = (union sctp_notification *)recvline;
	if(snp->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
		sac = &snp->sn_assoc_change;
		if((sac->sac_state == SCTP_COMM_UP) ||
		   (sac->sac_state == SCTP_RESTART)) {
			num_rem = sctp_getpaddrs(sock_fd,sac->sac_assoc_id,&sar);
			printf("There are %d remote addresses and they are:\n",
			       num_rem);
			sctp_print_addresses(sar,num_rem);
			sctp_freepaddrs(sar);

			num_loc = sctp_getladdrs(sock_fd,sac->sac_assoc_id,&sal);
			printf("There are %d local addresses and they are:\n",
			       num_loc);
			sctp_print_addresses(sal,num_loc);
			sctp_freeladdrs(sal);
		}
	}

}

int
sctpstr_cli(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
	struct sockaddr_in peeraddr;
	struct sctp_sndrcvinfo sri;
	char sendline[MAXLINE], recvline[MAXLINE];
	socklen_t len;
	int out_sz,rd_sz;
	int msg_flags;

	bzero(&sri,sizeof(sri));
	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if(sendline[0] != '[') {
			printf("Error, line must be of the form '[streamnum]text'\n");
			continue;
		}
		sri.sinfo_stream = strtol(&sendline[1],NULL,0);
		out_sz = strlen(sendline);
		if( sctp_sendmsg(sock_fd, sendline, out_sz, 
			     to, tolen, 
			     0, 0,
			     sri.sinfo_stream,
			     0, 0) <0 ){
		    fprintf(stderr,"sctp_sendmsg error : %s\n", strerror(errno));
		    return 1;
	    }

		do {
			len = sizeof(peeraddr);
			if((rd_sz = sctp_recvmsg(sock_fd, recvline, sizeof(recvline),
				     (SA *)&peeraddr, &len,
				     &sri,&msg_flags)) < 0 ) {
				fprintf(stderr,"sctp_recvmsg error : %s\n", strerror(errno));
				return 1;
			}
					 
			if(msg_flags & MSG_NOTIFICATION)
				check_notification(sock_fd,recvline,rd_sz);
		} while (msg_flags & MSG_NOTIFICATION);
		printf("From str:%d seq:%d (assoc:0x%x):",
		       sri.sinfo_stream,sri.sinfo_ssn,
		       (u_int)sri.sinfo_assoc_id);
		printf("%.*s",rd_sz,recvline);
		
	}
	return 0;
}



int
main(int argc, char **argv)
{
	int sock_fd;
	struct sockaddr_in6 servaddr;
	struct sctp_event_subscribe evnts;
	int echo_to_all=0;

	if(argc < 2){
		fprintf(stderr, "Missing host argument - use '%s host'\n",
		       argv[0]);
		return 1;
	}

    if ( (sock_fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(SERV_PORT);
	if( inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) < 1 ){
		fprintf(stderr,"inet_pton error : %s\n", strerror(errno));
		return 1;
	}

	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event = 1;
	evnts.sctp_association_event = 1;
	if( setsockopt(sock_fd,IPPROTO_SCTP, SCTP_EVENTS,
		   &evnts, sizeof(evnts)) > 0 ){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}
		   
	sctpstr_cli(stdin,sock_fd,(SA *)&servaddr,sizeof(servaddr));

	close(sock_fd);
	return(0);
}
