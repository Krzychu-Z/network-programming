#ifndef _TCP_HELPER_
#define _TCP_HELPER_ 

#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <sys/time.h>    /* includes <time.h> unsafely */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include 		<sys/socket.h>
#include 		<unistd.h>


#define SA struct sockaddr
#define LISTENQ 2

int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);
int tcp_accept( int listenfd);
int tcp_connect(const char *host, const char *serv);
int _signal(int signo, void  *func );
void Signal(int signo, void  *func);

ssize_t	writen(int fd, const void *vptr, size_t n); 	/* Write "n" bytes to a descriptor. */
void Writen(int fd, void *ptr, size_t nbytes);	
char *Fgets(char *ptr, int n, FILE *stream);
void Fputs(const char *ptr, FILE *stream);


#endif /* _TCP_HELPER_ */ 