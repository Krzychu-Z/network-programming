

#include	"ping.h"
#include 	<stdarg.h>
#include	<stdio.h>

struct proto	proto_v4 = { proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP };

#ifdef	IPV6
struct proto	proto_v6 = { proc_v6, send_v6, init_v6, NULL, NULL, 0, IPPROTO_ICMPV6 };
#endif


//#define printf

void err_quit(const char *fmt,...){
  va_list	ap;
  char buf[1024];
  int err = errno;
  strcpy(buf,"%s : ");
  strcat(buf, fmt);
  fprintf(stderr, buf, strerror(err), ap);
  exit(1);
}

void err_sys(const char *fmt,...){
  va_list	ap;
  char buf[1024];
  int err = errno;
  strcpy(buf,"%s : ");
  strcat(buf, fmt);
  fprintf(stderr, buf, strerror(err), ap);
  exit(2);
}


struct addrinfo *
Host_serv(const char *host, const char *serv, int family, int socktype)
{
	int				n;
	struct addrinfo	hints, *res;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
	hints.ai_family = family;		/* 0, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = socktype;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("host_serv error for %s, %s: %s",
				 (host == NULL) ? "(no hostname)" : host,
				 (serv == NULL) ? "(no service name)" : serv,
				 gai_strerror(n));

	return(res);	/* return pointer to first on linked list */
}

#ifdef	HAVE_SOCKADDR_DL_STRUCT
#include	<net/if_dl.h>
#endif

char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, sizeof(str), "%s", unp->sun_path);
		return(str);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;

		if (sdl->sdl_nlen > 0)
			snprintf(str, sizeof(str), "%*s",
					 sdl->sdl_nlen, &sdl->sdl_data[0]);
		else
			snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
		return(str);
	}
#endif
	default:
		snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
		err_sys("sock_ntop_host error");	/* inet_ntop() sets errno */
	return(ptr);
}

void
tv_sub(struct timeval *out, struct timeval *in)
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

int	datalen = 56;		/* data that goes with ICMP echo request */


int
main(int argc, char **argv)
{
	int				c;
	struct addrinfo	*ai;
	char *h;

	opterr = 0;		/* don't want getopt() writing to stderr */
	while ( (c = getopt(argc, argv, "v:i:")) != -1) {
		switch (c) {
		case 'v':
			verbose++;
			break;

		case 'i':
printf("I = %s\n", optarg);
			break;

		case '?':
			err_quit("unrecognized option: %c", c);
		}
	}

	if (optind != argc-1)
		err_quit("usage: ping [ -v ] <hostname>\n");
	host = argv[optind];

	pid = getpid() & 0xffff;	/* ICMP ID field is 16 bits */
	if ( (signal(SIGALRM, sig_alrm)) == SIG_ERR)
		err_sys("signal error");

	ai = Host_serv(host, NULL, 0, 0);

	h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
	printf("PING %s (%s): %d data bytes\n",
			ai->ai_canonname ? ai->ai_canonname : h,
			h, datalen);

		/* 4initialize according to protocol */
	if (ai->ai_family == AF_INET) {
		pr = &proto_v4;
#ifdef	IPV6
	} else if (ai->ai_family == AF_INET6) {
		pr = &proto_v6;
		if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)
								 ai->ai_addr)->sin6_addr)))
			err_quit("cannot ping IPv4-mapped IPv6 address");
#endif
	} else
		err_quit("unknown address family %d", ai->ai_family);

	pr->sasend = ai->ai_addr;
	pr->sarecv = calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;

	readloop();

	exit(0);
}
