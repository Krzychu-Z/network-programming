#include	"tcp_helper.h"

int
main(int argc, char **argv)
{
	int n, sockfd;

	if (argc != 3){
		fprintf(stderr,"usage: tcpsend01 <host> <port#>\n");
		return 1;
	}

	sockfd = tcp_connect(argv[1], argv[2]);

	n = write(sockfd, "123", 3);
	printf("wrote '123' - %d bytes of normal data\n", n);
	sleep(2);

//	n = send(sockfd, "45", 2, MSG_OOB);
//	printf("wrote '45' - %d bytes, including 1 byte of OOB data\n", n);
//	sleep(2);
	
	n = send(sockfd, "6", 1, MSG_OOB);
	printf("wrote '6' - %d byte of OOB data\n", n);
	sleep(1);

	n = write(sockfd, "789", 3);
	printf("wrote '789' - %d bytes of normal data\n", n);
	sleep(1);

	n = send(sockfd, "ABC", 3, MSG_OOB);
	printf("wrote 'ABC' - %d bytes, including 1 byte of OOB data\n", n);
	sleep(1);

	n = write(sockfd, "DE", 2);
	printf("wrote 'DE' - %d bytes of normal data\n", n);
	sleep(1);
	
	return(0);
}
