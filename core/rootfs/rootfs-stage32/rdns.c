#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr,"usage: %s <IPADDRESS>\n", argv[0]);
		return 1;
	}

	struct hostent *he;
	struct in_addr ipv4addr;
	struct in6_addr ipv6addr;

	inet_pton(AF_INET, argv[1], &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	if (he == NULL) return 2;
	if (he->h_name == NULL) return 3;
	printf("%s\n", he->h_name);

	return 0;
}

