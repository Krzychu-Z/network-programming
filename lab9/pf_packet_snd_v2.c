#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SA struct sockaddr

#define MYPTNO 0x0801

int main(int argc, char **argv)
{
	int	sockfd;
	struct sockaddr_ll servaddr, bindaddr;

	bzero(&servaddr, sizeof(servaddr));
	int if_idx;
	char name[]="wlp2s0";
	u_int8_t b0, b1, b2, b3, b4, b5;

	//destination's ifindex and addr is known

	if_idx =  if_nametoindex(name);
	if(argc < 2){
		printf("No MAC address given!\n");
		return -1;
	}
    printf("res = %d\n", sscanf(argv[1],"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &b0, &b1, &b2, &b3, &b4, &b5 ));
	servaddr.sll_family = AF_PACKET;
	servaddr.sll_ifindex = if_idx;
    servaddr.sll_protocol = htons(MYPTNO);
	servaddr.sll_halen = 6;
       
	servaddr.sll_addr[0] = b0;
	servaddr.sll_addr[1] = b1;
	servaddr.sll_addr[2] = b2;
	servaddr.sll_addr[3] = b3;
	servaddr.sll_addr[4] = b4;
	servaddr.sll_addr[5] = b5;

	sockfd = socket(PF_PACKET, SOCK_DGRAM, htons(MYPTNO));
//	sockfd = socket(PF_PACKET, SOCK_RAW, htons(MYPTNO));

    if( sockfd < 0 )
        perror("socket error");
	char buffer[512];

	sprintf(buffer, "---Alfa IP protocol version 0.001---");
	if( sendto(sockfd, buffer, strlen(buffer)+1, 0, (SA*)&servaddr, sizeof(servaddr)) < 0 )
                perror("sendto error");

	exit(0);
}

