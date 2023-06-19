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
        char name[]="eth0";

        //destination's ifindex and addr is known

        if_idx =  if_nametoindex(name);

	servaddr.sll_family = AF_PACKET;
	servaddr.sll_ifindex = if_idx;
        servaddr.sll_protocol = htons(MYPTNO);
	servaddr.sll_halen = 6;
        //v131 24:b6:fd:14:02:e0,  80:86:f2:09:1f:dc
	servaddr.sll_addr[0] = 0x24;
	servaddr.sll_addr[1] = 0xb6;
	servaddr.sll_addr[2] = 0xfd;
	servaddr.sll_addr[3] = 0x14;
	servaddr.sll_addr[4] = 0x02;
	servaddr.sll_addr[5] = 0xe0;

	sockfd = socket(PF_PACKET, SOCK_DGRAM, htons(MYPTNO));

        if( sockfd < 0 )
                perror("socket error");
	char buffer[512];

	sprintf(buffer, "---Alfa IP protocol version 0.001---");
	if( sendto(sockfd, buffer, strlen(buffer)+1, 0, (SA*)&servaddr, sizeof(servaddr)) < 0 )
                perror("sendto error");

	exit(0);
}

