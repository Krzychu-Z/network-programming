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


sctp_assoc_t
sctp_address_to_associd(int sock_fd, struct sockaddr *sa, socklen_t salen, sctp_assoc_t assoc_id)
{
	struct sctp_paddrparams sp;
	int siz;
/*
struct sctp_paddrparams {
    sctp_assoc_t               spp_assoc_id;
    struct sockaddr_storage    spp_address;
    uint32_t                   spp_hbinterval;
    uint16_t                   spp_pathmaxrxt;
};
*/
		
	siz = sizeof(struct sctp_paddrparams);
	bzero(&sp,siz);
	memcpy(&sp.spp_address,sa,salen);
	sctp_opt_info(sock_fd,0,
		   SCTP_PEER_ADDR_PARAMS, &sp, &siz);
	return(sp.spp_assoc_id);
}


int 
sctp_get_no_strms(int sock_fd, sctp_assoc_t assoc_id)
{
	int retsz;
	struct sctp_status status;
/* struct sctp_status {
        sctp_assoc_t            sstat_assoc_id;
        __s32                   sstat_state;
        __u32                   sstat_rwnd;
        __u16                   sstat_unackdata;
        __u16                   sstat_penddata;
        __u16                   sstat_instrms;
        __u16                   sstat_outstrms;
        __u32                   sstat_fragmentation_point;
        struct sctp_paddrinfo   sstat_primary;
};*/	
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
sctp_get_no_strms2(int sock_fd, sctp_assoc_t assoc_id)
{
	int retsz;
	struct sctp_status status;
	retsz = sizeof(status);	
	bzero(&status,sizeof(status));

	status.sstat_assoc_id = assoc_id;
	if( getsockopt(sock_fd,IPPROTO_SCTP, SCTP_STATUS,
		   &status, &retsz) <0 ){
		perror("sctp_get_no_strms:getsockopt error");
		exit(-1);
	}
	return(status.sstat_outstrms);
}

void
print_notification(char *notify_buf)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
	struct sctp_adaptation_event *ae;
	struct sctp_pdapi_event *pdapi;
	const char *str;

	snp = (union sctp_notification *)notify_buf;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {
		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			break;
		case SCTP_RESTART:
			str = "RESTART";
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CAN'T START ASSOC";
			break;
		default:
			str = "UNKNOWN";
			break;
		} /* end switch(sac->sac_state) */
		printf("SCTP_ASSOC_CHANGE: %s, assoc=0x%x\n", str,
		       (uint32_t)sac->sac_assoc_id);
		break;
	case SCTP_PEER_ADDR_CHANGE:
	/*
	enum sctp_spc_state {
        SCTP_ADDR_AVAILABLE,
        SCTP_ADDR_UNREACHABLE,
        SCTP_ADDR_REMOVED,
        SCTP_ADDR_ADDED,
        SCTP_ADDR_MADE_PRIM,
        SCTP_ADDR_CONFIRMED,
};

	*/
		spc = &snp->sn_paddr_change;
		switch(spc->spc_state) {
		case SCTP_ADDR_AVAILABLE:
			str = "ADDRESS AVAILABLE";
			break;
		case SCTP_ADDR_UNREACHABLE:
			str = "ADDRESS UNREACHABLE";
			break;
		case SCTP_ADDR_REMOVED:
			str = "ADDRESS REMOVED";
			break;
		case SCTP_ADDR_ADDED:
			str = "ADDRESS ADDED";
			break;
		case SCTP_ADDR_MADE_PRIM:
			str = "ADDRESS MADE PRIMARY";
			break;
		case SCTP_ADDR_CONFIRMED:
			str = "SCTP ADDR CONFIRMED";
			break;
		default:
			str = "UNKNOWN";
			break;
		} /* end switch(spc->spc_state) */
		printf("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=0x%x\n", str,
		       sock_ntop((SA *)&spc->spc_aaddr, sizeof(spc->spc_aaddr)),
		       (uint32_t)spc->spc_assoc_id);
		break;
	case SCTP_REMOTE_ERROR:
		sre = &snp->sn_remote_error;
		printf("SCTP_REMOTE_ERROR: assoc=0x%x error=%d\n",
		       (uint32_t)sre->sre_assoc_id, sre->sre_error);
		break;
	case SCTP_SEND_FAILED:
		ssf = &snp->sn_send_failed;
		printf("SCTP_SEND_FAILED: assoc=0x%x error=%d\n",
		       (uint32_t)ssf->ssf_assoc_id, ssf->ssf_error);
		break;
	case SCTP_ADAPTATION_INDICATION:
		ae = &snp->sn_adaptation_event;
		printf("SCTP_ADAPTATION_INDICATION: 0x%x\n",
		    (u_int)ae->sai_adaptation_ind);
		break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	    pdapi = &snp->sn_pdapi_event;
	    if(pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED)
		    printf("SCTP_PARTIAL_DELIEVERY_ABORTED\n");
	    else
		    printf("Unknown SCTP_PARTIAL_DELIVERY_EVENT 0x%x\n",
			   pdapi->pdapi_indication);
	    break;
	case SCTP_SHUTDOWN_EVENT:
		sse = &snp->sn_shutdown_event;
		printf("SCTP_SHUTDOWN_EVENT: assoc=0x%x\n",
		       (uint32_t)sse->sse_assoc_id);
		break;
	default:
		printf("Unknown notification event type=0x%x\n", 
		       snp->sn_header.sn_type);
	}
}




int
main(int argc, char **argv)
{
	int sock_fd,msg_flags;
	char readbuf[BUFFSIZE];
	struct sockaddr_in6 servaddr, cliaddr;
	struct sctp_sndrcvinfo sri;
	struct sctp_event_subscribe evnts;
	int stream_increment=1;
	socklen_t len;
	size_t rd_sz;

	if (argc == 2)
		stream_increment = atoi(argv[1]);
		
    if((sock_fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0 ){
		perror("socket error");
		exit(-1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port = htons(SERV_PORT);

	if( bind(sock_fd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		perror("bind error");
		exit(-1);
	}

/*
struct sctp_event_subscribe {
        __u8 sctp_data_io_event;
        __u8 sctp_association_event;
        __u8 sctp_address_event;
        __u8 sctp_send_failure_event;
        __u8 sctp_peer_error_event;
        __u8 sctp_shutdown_event;
        __u8 sctp_partial_delivery_event;
        __u8 sctp_adaptation_layer_event;
        __u8 sctp_authentication_event;
        __u8 sctp_sender_dry_event;
};

*/
	
	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event = 1;
	evnts.sctp_association_event = 1;
	evnts.sctp_address_event = 1;
	evnts.sctp_send_failure_event = 1;
	evnts.sctp_peer_error_event = 1;
	evnts.sctp_shutdown_event = 1;
	evnts.sctp_partial_delivery_event = 1;
	evnts.sctp_adaptation_layer_event = 1;
	if( setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
		   &evnts, sizeof(evnts)) <0 ){
		perror("setsockopt error");
		exit(-1);
	}

	if( listen(sock_fd, LISTENQ) < 0 ){
		perror("listen error");
		exit(-1);
	}
	for ( ; ; ) {
		len = sizeof(struct sockaddr_in6);
		if( (rd_sz = sctp_recvmsg(sock_fd, readbuf, sizeof(readbuf),
			     (SA *)&cliaddr, &len,
			     &sri,&msg_flags))  < 0){
			perror("sctp_recvmsg error");
//			exit(-1);
		}		 
		if(msg_flags & MSG_NOTIFICATION) {
			print_notification(readbuf);
			continue;
		}
		
		printf("Connection from: %s [assoc id = 0x%x]\n", sock_ntop((SA*)&cliaddr,len), 
			sri.sinfo_assoc_id);
/*
		struct sctp_sndrcvinfo {
    uint16_t        sinfo_stream;
    uint16_t        sinfo_ssn;
    uint16_t        sinfo_flags;
    uint32_t        sinfo_ppid;
    uint32_t        sinfo_context;
    uint32_t        sinfo_timetolive;
    uint32_t        sinfo_tsn;
    uint32_t        sinfo_cumtsn;
    sctp_assoc_t    sinfo_assoc_id;
};*/
		
		if(stream_increment) {
			sri.sinfo_stream++;
			if(sri.sinfo_stream >= sctp_get_no_strms(sock_fd, sri.sinfo_assoc_id)) 
				sri.sinfo_stream = 0;
		}
		rd_sz = sctp_sendmsg(sock_fd, readbuf, rd_sz, 
			     (SA *)&cliaddr, len,
			     sri.sinfo_ppid,
			     sri.sinfo_flags,
			     sri.sinfo_stream,
			     0, 0);
		if( rd_sz < 0 ){
			perror("sctp_sendmsg error");
//			exit(-1);
		}		 

    }
}
