#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>


#define SA struct sockaddr
#define MYPTNO 0x0801
//#define MYPTNO ETH_P_PUP  
//ETH_P_ALL
int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_ll	servaddr, cliaddr;
	int if_idx=0;
	char name[]="wlan0";

    //destination's ifindex and addr is known

    if_idx =  if_nametoindex(name);

//	sockfd = socket(PF_PACKET, SOCK_DGRAM, htons(MYPTNO));
	sockfd = socket(PF_PACKET, SOCK_RAW, htons(MYPTNO));
	if( sockfd < 0 ){
		perror("socket error:");
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sll_family = AF_PACKET;
	servaddr.sll_protocol = htons(MYPTNO);
//	servaddr.sll_ifindex = 0;//bind to every interface
	servaddr.sll_ifindex = if_idx;

//	if( bind(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0 )
//	 	perror("bind error:");

	bzero(&cliaddr, sizeof(cliaddr));

	char buff[512];
	int n = 0;
	
	for(;;){
		int length=sizeof(cliaddr);
//		n = recvfrom(sockfd, buff, 512, 0, (SA*)&cliaddr, &length);
		n = read(sockfd, buff, 512);
		if( n < 0 )
			perror("socket error:");
		else{
			buff[n] = 0;

			// struct ethhdr {
			//         unsigned char   h_dest[ETH_ALEN];       /* destination eth addr */
			//         unsigned char   h_source[ETH_ALEN];     /* source ether addr    */
			//         __be16          h_proto;                /* packet type ID field */
			// } __attribute__((packed));

			struct ethhdr *hdr;
			hdr = (struct ethhdr *)buff;
			printf("SRC MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
				(int) hdr->h_source[0], (int) hdr->h_source[1], (int) hdr->h_source[2],
				(int) hdr->h_source[3], (int) hdr->h_source[4],(int) hdr->h_source[5] );
			printf("DST MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
				(int) hdr->h_dest[0], (int) hdr->h_dest[1], (int) hdr->h_dest[2],
				(int) hdr->h_dest[3], (int) hdr->h_dest[4],(int) hdr->h_dest[5] );
			printf("Proto = 0x%04x\n", ntohs( hdr->h_proto));

			printf("DATA = %s\n", buff+sizeof(struct ethhdr));
		}
	}

	return 0;
}
