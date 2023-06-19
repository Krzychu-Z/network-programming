#include    <sys/types.h>   /* basic system data types */
#include    <sys/socket.h>  /* basic socket definitions */
#include    <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <errno.h>
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include 	<sys/ioctl.h>
#include 	<unistd.h>
#include 	<net/if.h>
#include 	<net/if_arp.h>


#define MAXLINE 1024
#define SA      struct sockaddr

unsigned int
_if_nametoindex(const char *ifname)
{
	int s;
	struct ifreq ifr;
	unsigned int ni;

	s = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (s != -1) {

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	
	if (ioctl(s, SIOCGIFINDEX, &ifr) != -1) {
			close(s);
			return (ifr.ifr_ifindex);
	}
		close(s);
		return -1;
	}
}

// #include <net/if.h>
// unsigned int if_nametoindex(const char *ifname);
// char *if_indextoname(unsigned int ifindex, char *ifname);


int get_mac_addr( char* name, char* mac )
{
	struct ifreq ifr;
	int sd, merr;
	sd = socket ( PF_INET, SOCK_STREAM, 0 );
	if ( sd < 0 )
	{	printf("if_up: socket error: %s ", strerror ( errno ) );
		return sd;
	}
	memset ( &ifr, 0, sizeof ( ifr ) );
	sprintf ( ifr.ifr_name, "%s", name );
	merr=ioctl ( sd, SIOCGIFHWADDR, &ifr ); /* Read MAC address */
	if ( merr < 0 )
	{    	close ( sd );
		return merr;
	}
	close ( sd ); 
	char buf[4000];
	sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x\n", 
		(int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[0],   (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[1],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[2],   (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[3],
      (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[4],   (int) ((unsigned char *) &ifr.ifr_hwaddr.sa_data)[5]);

   if( mac == NULL)
  	printf(" Device %s -> MAC = %s", ifr.ifr_name, buf);
   else
 	strcpy(mac,buf); 
   return 0;
}


int set_mac_addr( char* name, char* mac)
{
	struct ifreq ifr;
	int sd, merr;
	sd = socket ( PF_INET, SOCK_STREAM, 0 );
	if ( sd < 0 )
	{	printf("if_up: socket error: %s ", strerror ( errno ) );
		return sd;
	}
	memset ( &ifr, 0, sizeof ( ifr ) );
	sprintf ( ifr.ifr_name, "%s", name );
	
    sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", //24:b6:fd:14:02:e0
        &ifr.ifr_hwaddr.sa_data[0],
        &ifr.ifr_hwaddr.sa_data[1],
        &ifr.ifr_hwaddr.sa_data[2],
        &ifr.ifr_hwaddr.sa_data[3],
        &ifr.ifr_hwaddr.sa_data[4],
        &ifr.ifr_hwaddr.sa_data[5]
    );
 
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	merr=ioctl ( sd, SIOCSIFHWADDR, &ifr ); /* Set MAC address */
	if ( merr < 0 )
	{    	close ( sd );
		return merr;
	}
	close ( sd ); 
	return 0;
}


int main(int argc, char* argv[]){
  
	unsigned int idx;  
   
    if( argc < 2 ){
      printf("USAGE: %s interface_name\n", argv[0]);
      exit(1);
    }

	if( argc == 2 ){	
		idx = _if_nametoindex(argv[1]);
		if( idx != -1 )
			printf("Index of interface %s = %u (%u)\n", argv[1], idx, if_nametoindex(argv[1]));
		else
			printf("_if_nametoindex error: %s \n", strerror ( errno ) );
		
		if( get_mac_addr( argv[1], NULL ) < 0 )
			printf("get_mac_addr error: %s \n", strerror ( errno ) );
	}
	
	if( argc == 3 )
		if( set_mac_addr( argv[1], argv[2] ) < 0 )
			printf("set_mac_addr error: %s \n", strerror ( errno ) );;
	
	if( argc > 3 )
		printf("Unsupported operation \n");
	
    return 0;
}
