#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints = { 0 };
    struct addrinfo *res, *res_c;
    int err;
    char name[INET6_ADDRSTRLEN];

    if (argc < 2)
    {
        return 1;
    }

//    hints.ai_family = AF_INET6;
    hints.ai_family = AF_UNSPEC;
 //   hints.ai_flags =  AI_ALL;
    hints.ai_flags = AI_V4MAPPED | AI_ALL;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);

    if (err)
    {
        printf("getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    for (res_c = res; res_c; res_c = res_c->ai_next)
    {
        const void *addr;
        int port;
        struct protoent *proto;

        switch (res_c->ai_family)
        {
            case AF_INET6:
                addr = &((struct sockaddr_in6 *)(res_c->ai_addr))->sin6_addr;
                port = ((struct sockaddr_in6 *)(res_c->ai_addr))->sin6_port;
                printf("AF_INET6\t");
                break;
            case AF_INET:
                addr = &((struct sockaddr_in *)(res_c->ai_addr))->sin_addr;
                port = ((struct sockaddr_in *)(res_c->ai_addr))->sin_port;
                printf("AF_INET\t");
                break;
            default:
                addr = NULL;
                printf("(%d)\t", res_c->ai_family);
        }

        proto = getprotobynumber(res_c->ai_protocol);
        if (proto)
        {
            printf("%s\t", proto->p_name);
        }
        else
        {
            printf("(%d)\t", res_c->ai_protocol);
        }

        switch (res_c->ai_socktype)
        {
            case SOCK_STREAM:
                printf("SOCK_STREAM\t");
                break;

            case SOCK_DGRAM:
                printf("SOCK_DGRAM\t");
                break;

            case SOCK_SEQPACKET:
                printf("SOCK_SEQPACKET\t");
                break;

            case SOCK_RAW:
                printf("SOCK_RAW\t");
                break;

            default:
                printf("(?socktype?)\t");
                break;
        }

        if (addr && inet_ntop(res_c->ai_family, addr, name, sizeof name))
            printf("addr = %s", name);

        if (addr)
            printf(",%d", ntohs(port));

        printf("\n");
    }

    return 0;
}
