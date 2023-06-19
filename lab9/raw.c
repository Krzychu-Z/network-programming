#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <ctype.h>


#define SA struct sockaddr
//#define MYPTNO 0x0800 //IPv4
#define MYPTNO 0 
//#define MYPTNO IPPROTO_UDP //IPv4
//#define MYPTNO ETH_P_ALL
//#define MYPTNO ETH_P_IPV6

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_ll	servaddr, cliaddr;
    int if_idx;

    char *name=argv[1];


	if (argc != 2){
		fprintf(stderr, "usage: %s <Interface name> \n", argv[0]);
		return 1;
	}

    //destination's ifindex and addr is known

    if( (if_idx =  if_nametoindex(name)) == 0 ){
    	perror("interface name error:");
    	return 1;
    }

//	sockfd = socket(AF_INET, SOCK_RAW, htons(MYPTNO));
	sockfd = socket(AF_INET6, SOCK_RAW, MYPTNO);
//	sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_TCP);
//	sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
	if( sockfd < 0 ){
		perror("socket error:");
		exit(-1);
	}

//	bzero(&servaddr, sizeof(servaddr));
//	servaddr.sll_family = AF_PACKET;
//	servaddr.sll_protocol = htons(MYPTNO);
//	servaddr.sll_ifindex = 0;//bind to every interface
//	servaddr.sll_ifindex = if_idx;

//	 if( bind(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0 )
//	 	perror("bind error:");

	bzero(&cliaddr, sizeof(cliaddr));

	char buff[2048];
	int n = 0;
	int i=0,j =0;
	for(;;){
		int length=sizeof(cliaddr);
		n = recvfrom(sockfd, buff, 2048, 0, (SA*)&cliaddr, &length);
		if( n < 0 )
			perror("recvfrom error:");
		else{
			buff[n] = 0;
			j++;

			struct ip *ipv4hdr;
			struct udphdr *uhdr;
			ipv4hdr = (struct ip *)(buff);
//			uhdr = (struct udphdr *)(buff+sizeof(struct ip));
			uhdr = (struct udphdr *)(buff);
  			printf("test IPv6:\n ");
			fflush(stdout);

			if( (ipv4hdr->ip_p == IPPROTO_TCP) || (ipv4hdr->ip_p == IPPROTO_ICMP) || (ipv4hdr->ip_p == IPPROTO_UDP)  ){

				printf("IP src addr = %s\n", inet_ntoa( ipv4hdr->ip_src));
				printf("IP dst addr = %s\n", inet_ntoa( ipv4hdr->ip_dst));
				if( ipv4hdr->ip_p == IPPROTO_UDP  ){
					printf("UDP src port = %u\n", ntohs( uhdr->uh_sport));
					printf("UDP dst port = %u\n", ntohs( uhdr->uh_dport));
				}

				char *out=buff+sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct udphdr);
				}
					printf("UDP src port = %u\n", ntohs( uhdr->uh_sport));
					printf("UDP dst port = %u\n", ntohs( uhdr->uh_dport));
				printf("DATA = ");
				int k=0;
				
				for(k=0; k< n; k++){
					if((isprint(buff[k])))
						printf("%c",buff[k] );
					else
						printf("-");		
				}
			
			}
			printf("\n");
			fflush(stdout);
			i++;
			if( i > 10)
				break;
	}

	printf("\nReceived %d packets\n",j);

	return 0;
}

