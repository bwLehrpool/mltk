/************************
 * sslconnect 0.2
 * Last Change: 2013-06-17
 * C Implementation by Simon Rettberg
 * Original sslconnect 0.1 was written in perl by Martin Walter
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* Init libs and data strctures */
void init();
/* print error report of something failed */
void ssl_error();
/* connect via ssl */
SSL* ssl_connect(char * host, uint16_t port, uint16_t local_port, SSL_CTX ** ctx);
/* read from ssl connection */
ssize_t ssl_read(SSL * bio, char * buffer, ssize_t length);
/* write to ssl connection */
int ssl_write(SSL * bio, char * buffer, ssize_t length);

int main(int argc, char ** argv);

void init()
{
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
}

void ssl_error(char* message)
{
	fprintf(stderr, message);
	fprintf(stderr, "\n%s\n", ERR_error_string(ERR_get_error(), NULL));
	fprintf(stderr, "Details: %s\n", ERR_reason_error_string(ERR_get_error()));
	ERR_print_errors_fp(stderr);
}

SSL* ssl_connect(char * host, uint16_t port, uint16_t local_port, SSL_CTX ** ctx)
{
	int ret = 0;
	/* create socket. needs to be done manually in order to bind to local port */
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "Could not create socket.\n");
		return NULL;
	}

	struct sockaddr_in sa_dest, sa_local;
	memset(&sa_local, 0, sizeof(sa_local));
	memset(&sa_dest, 0, sizeof(sa_dest));

	sa_local.sin_family = AF_INET;
	sa_local.sin_port = htons(local_port);
	ret = bind(fd, (struct sockaddr *)&sa_local, sizeof(struct sockaddr));
	if (ret == -1) {
		fprintf(stderr, "Could not bind local socket to 0.0.0.0:%d (%d)\n", (int)local_port, (int)errno);
		close(fd);
		return NULL;
	}

	sa_dest.sin_family = AF_INET;
	sa_dest.sin_port = htons(port);
	struct hostent * rec;
	rec = gethostbyname(host);
	if (rec == NULL) {
		fprintf(stderr, "Error: Invalid host: %s\n", host);
		return NULL;
	}
	memcpy(&(sa_dest.sin_addr), rec->h_addr, sizeof(struct in_addr));

	ret = connect(fd, (struct sockaddr *)&sa_dest, sizeof(struct sockaddr));
	if (ret == -1) {
		fprintf(stderr, "Could not connect to %s:%d (%d)\n", host, (int)port, (int)errno);
		close(fd);
		return NULL;
	}

	/* openssl part */
	SSL * ssl;

	/* Set up the SSL pointers */
	*ctx = SSL_CTX_new(SSLv23_client_method());
	ssl = SSL_new(*ctx);
	SSL_set_fd(ssl, fd);
	ret = SSL_connect(ssl);

	if (ret <= 0) {
		ssl_error("Unable to SSL_connect");
		return NULL;
	}

	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	return ssl;
}

ssize_t ssl_read(SSL * ssl, char * buffer, ssize_t length)
{
	ssize_t ret = -1;
	int retries = 10;

	while (ret < 0 && --retries > 0) {

		ret = SSL_read(ssl, buffer, length);
		if (ret >= 0) {
			return ret;
		}

		ssl_error("SSL_read failed");
		return -1;

	}

	return -1;
}

int ssl_write(SSL * ssl, char * buffer, ssize_t length)
{
	ssize_t ret = -1;
	int retries = 10;

	while (ret < 0 && --retries > 0) {

		ret = SSL_write(ssl, buffer, length);
		if (ret >= 0) {
			return ret;
		}

		ssl_error("SSL_write failed");
		return -1;

	}

	return -1;
}

#define READBUF 5000
int main(int argc, char ** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s host:port\n", argv[0]);
		return 1;
	}

	init();

	char buffer[READBUF];
	SSL_CTX * ctx = NULL;
	SSL * ssl;
	ssize_t len;
	size_t ret;
	char * pos;
	int port, lport;
	pos = strchr(argv[1], ':');
	if (pos == NULL) {
		fprintf(stderr, "Error: No Port given.\n");
		return 5;
	}
	port = atoi(pos+1);
	*pos = '\0';

	lport = rand() % 800 + 95;

	ssl = ssl_connect(argv[1], (uint16_t)port, (uint16_t)lport, &ctx);
	if (ssl == NULL) {
		return 2;
	}

	ssl_write(ssl, "", 0);
	for (;;) {
		len = ssl_read(ssl, buffer, READBUF);
		if (len <= 0) {
			break;
		}
		ret = fwrite(buffer, 1, len, stdout);
		if (ret != len) {
			fprintf(stderr, "Error: fwrite could not write all received data to stdout.\n");
			return 3;
		}
	}

	if (len < 0) {
		return 4;
	}

	return 0;
}

