#include <pcap.h>
#include <pcap/pcap.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <ctype.h>

#define SNAP_LEN 100

const char *_inet_ntop( struct sockaddr  *pcliaddr, char *str, int len)
{
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;

	if(pcliaddr == NULL)
		return NULL;

	if( pcliaddr->sa_family == AF_INET6 ){
		cliaddr = (struct sockaddr_in6*) pcliaddr;
		      return inet_ntop(AF_INET6,  (struct sockaddr  *) &cliaddr->sin6_addr,  str, len);
	}
	else{
		      cliaddrv4 = (struct sockaddr_in*) pcliaddr;
		      return inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  str, len);
	}
}

int main(int argc, char *argv[])
{
	pcap_t *handle;			/* Session handle */
	char *dev;			/* The device to sniff on */
	char errbuf[PCAP_ERRBUF_SIZE];	/* Error string */
	struct bpf_program fp;		/* The compiled filter */
	char filter_exp[1024] = "udp";	/* The filter expression */
	bpf_u_int32 mask;		/* Our netmask */
	bpf_u_int32 net;		/* Our IP */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const u_char *buff;		/* The actual packet */
	//u_char *buff;		/* The actual packet */
	u_char out[2048];
	char str1[INET6_ADDRSTRLEN], str2[INET6_ADDRSTRLEN];
	pcap_if_t *alldevsp=NULL, *devsp=NULL;
	pcap_addr_t *p_addr;
	int n;
	int datalink=0;


	/* Define the device */
	dev = pcap_lookupdev(errbuf);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return(2);
	}
	printf("Default device to sniff is %s\n", dev);
	
	/* Find the properties for the default device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
		net = 0;
		mask = 0;
	}
	printf("localnet = %s, netmask = %s\n",
		inet_ntop(AF_INET, &net, str1, sizeof(str1)),
		inet_ntop(AF_INET, &mask, str2, sizeof(str2)));

	/*Find all devices*/
	printf("Other devices to be used are: \n");

	if( pcap_findalldevs( &alldevsp, errbuf) == -1 ) {
		fprintf(stderr, "Couldn't get devices %s\n", errbuf);
		return(2);
	}else{
		devsp = alldevsp;
		while( devsp != NULL ){
			fprintf(stdout, "\tDevice name %s [%s]\n", devsp->name, devsp->description );
			p_addr = devsp->addresses;
			while( p_addr != NULL ){
				printf("\t   localnet = %s, netmask = %s\n",       
					_inet_ntop(p_addr->addr, str1, sizeof(str1)),
					_inet_ntop(p_addr->netmask, str2, sizeof(str2)));
				p_addr=p_addr->next;
			}
			devsp=devsp->next;
			printf("\n");
		}
 		pcap_freealldevs(alldevsp);
	}

	if ( (argc != 2) && (argc != 3) ){
		fprintf(stderr, "usage: %s <Interface name> '<filter>'\n", argv[0]);
		return 1;
	}
	
	if( argc == 3)
		strncpy(filter_exp, argv[2], 2048);

	/* Open the session in promiscuous mode */
	handle = pcap_open_live(argv[1], SNAP_LEN, 1, 2000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return(2);
	}

	if (pcap_set_promisc(handle, 1) == -1) {
		fprintf(stderr, "Couldn't set promisc mode:  %s\n", pcap_geterr(handle));
		return(2);
	}

	printf("Link header = %s\n", pcap_datalink_val_to_name( pcap_datalink(handle)));
	if( pcap_set_datalink(handle,DLT_EN10MB) == -1 ){
			;
	}
	/* Compile and apply the filter */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	/* Grab a packet */

	int i=0, j=0;
	for(;;){
		
		buff = pcap_next(handle, &header);
		if(buff == NULL)
			continue;
		j++;
		datalink = pcap_datalink(handle);
		if ( datalink != DLT_EN10MB){
			printf("Link header = %s\n", pcap_datalink_val_to_name( datalink));
			continue;
		}

		/* Print its length */
		printf("\nIntercepted packet with length of %d [%d]\n", header.caplen, header.len);
		n= header.len;
		// struct ethhdr {
		//         unsigned char   h_dest[ETH_ALEN];       /* destination eth addr */
		//         unsigned char   h_source[ETH_ALEN];     /* source ether addr    */
		//         __be16          h_proto;                /* packet type ID field */
		// } __attribute__((packed));

		struct ethhdr *hdr;
		struct ip *ipv4hdr;
		struct udphdr *uhdr;
		hdr = (struct ethhdr *)buff;
		ipv4hdr = (struct ip *)(buff+sizeof(struct ethhdr));
		uhdr = (struct udphdr *)(buff+sizeof(struct ethhdr)+sizeof(struct ip));

		printf("SRC MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
			(int) hdr->h_source[0], (int) hdr->h_source[1], (int) hdr->h_source[2],
			(int) hdr->h_source[3], (int) hdr->h_source[4],(int) hdr->h_source[5] );
		printf("DST MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
			(int) hdr->h_dest[0], (int) hdr->h_dest[1], (int) hdr->h_dest[2],
			(int) hdr->h_dest[3], (int) hdr->h_dest[4],(int) hdr->h_dest[5] );
		printf("Proto = 0x%04x\n", ntohs( hdr->h_proto));

		if( ntohs (hdr->h_proto) == ETHERTYPE_IP ) {

			printf("IP src addr = %s\n", inet_ntoa( ipv4hdr->ip_src));
			printf("IP dst addr = %s\n", inet_ntoa( ipv4hdr->ip_dst));

			if( ipv4hdr->ip_p == IPPROTO_UDP ){

				printf("UDP src port = %u\n", ntohs( uhdr->uh_sport));
				printf("UDP dst port = %u\n", ntohs( uhdr->uh_dport));

				
				memcpy(out,buff+sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct udphdr), 
					(n-sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct udphdr)) );
					
				int k=0;
				printf("DATA = ");
				for(k=0; k< (n-sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct udphdr)); k++){
					if((isprint(out[k])))
						printf("%c",out[k] );
					else
						printf("-");		
				}
				i++;
				if( i > 4)
					break;
			}
			else
				printf("IP PROTO = %d", ipv4hdr->ip_p );
		}
		printf("\n");
		fflush(stdout);

	}
	printf("Sniffed %d packets\n",j);
	/* And close the session */
	pcap_close(handle);
	return(0);
}
