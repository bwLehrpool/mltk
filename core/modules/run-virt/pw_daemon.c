#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/random.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/un.h>
#include <grp.h>

static const ssize_t KEYLEN = 16;

static pid_t udpPid = -1;
static char *username = NULL;
static uint8_t *passwordEnc = NULL;
static size_t passwordLen = 0;
static uint8_t *key1 = NULL, *key2 = NULL;
static char *key1s = NULL, *key2s = NULL;

static int mode_daemon(const uid_t uidNumber);
static int mode_query(const char *socketPath);
static int mode_pw(const char *socketPath);
static void sig_handler(int sig);
static int setup_vars(const char *envuser, const char *envpass);
static uint8_t* keygen();
static char* bin2hex(uint8_t* bin, size_t len);
static uint8_t* xorString(const char* inputText, const uint8_t* key);
static int init_udp();

int main(int argc, char **argv)
{
	if (argc > 2 && strcmp(argv[1], "--daemon") == 0) {
		char *end = NULL;
		uid_t uid = (uid_t)strtoul(argv[2], &end, 10);
		if (argv[2][0] == '\0' || *end != '\0') {
			fprintf(stderr, "Invalid uidNumber\n");
			return 1;
		}
		return mode_daemon(uid);
	} else if (argc > 2 && strcmp(argv[1], "--query") == 0) {
		return mode_query(argv[2]);
		/*
	} else if (argc > 2 && strcmp(argv[1], "--pw") == 0) {
		return mode_pw(argv[2]);
		*/
	}
	fprintf(stderr, "Invalid call. Use --daemon [uidNumber] or --query [unixSocket]\n");
	return 1;
}

static int connect_local(const char *socketPath, int quiet)
{
	struct sockaddr_un remote;
	struct timeval tv;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		if (!quiet) {
			perror("Cannot create unix socket for connecting");
		}
		return -1;
	}
	memset(&remote, 0, sizeof(remote));
	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, socketPath, sizeof(remote.sun_path)-1);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	if (connect(fd, (struct sockaddr*)&remote, sizeof(remote)) == -1) {
		if (!quiet) {
			perror("Cannot connect to pw daemon");
		}
		close(fd);
		return -1;
	}
	return fd;
}

static int mode_query(const char *socketPath)
{
	int fd;
	char buffer[200];
	ssize_t ret;
	fd = connect_local(socketPath, 0);
	if (fd == -1)
		return 1;
	if (write(fd, "GET", 3) == -1) {
		perror("Writing to pw daemon failed");
		return 1;
	}
	ret = read(fd, buffer, sizeof(buffer)-1);
	if (ret == -1) {
		perror("Reading from pw daemon failed");
		return 1;
	}
	if (ret < 1 || (size_t)ret > sizeof(buffer)-1) {
		fprintf(stderr, "Reply from pw daemon has invalid length\n");
		return 1;
	}
	if (buffer[ret-1] != '\n') {
		fprintf(stderr, "Corrupted reply received from pw daemon\n");
		return 1;
	}
	buffer[ret] = '\0';
	printf("%s", buffer);
	return 0;
}

static int mode_pw(const char *socketPath)
{
	int fd;
	char buffer[200];
	ssize_t ret;
	fd = connect_local(socketPath, 0);
	if (fd == -1)
		return 1;
	if (write(fd, "PW", 3) == -1) {
		perror("Writing to pw daemon failed");
		return 1;
	}
	ret = read(fd, buffer, sizeof(buffer)-1);
	if (ret == -1) {
		perror("Reading from pw daemon failed");
		return 1;
	}
	if (ret < 1 || (size_t)ret > sizeof(buffer)-1) {
		fprintf(stderr, "Reply from pw daemon has invalid length\n");
		return 1;
	}
	buffer[ret] = '\0';
	printf("%s", buffer);
	return 0;
}

static int mode_daemon(const uid_t uidNumber)
{
	int listenFd, udpPort = -1, testFd;
	struct sockaddr_un addr;
	struct sigaction sig;
	const char *envuser = getenv("USERNAME");
	volatile char *envpass = getenv("PASSWORD");
	const char *pwsocket = getenv("PWSOCKET");
	const char *localstr = getenv("LOCAL_PW");
	int allowLocal = localstr != NULL && atoi(localstr);
	gid_t gidNumber = 65534;

	memset(&addr, 0, sizeof(addr));
	memset(&sig, 0, sizeof(sig));
	if (envuser == NULL) {
		fprintf(stderr, "USERNAME not set\n");
		return 1;
	}
	if (envpass == NULL) {
		fprintf(stderr, "PASSWORD not set\n");
		return 1;
	}
	if (pwsocket == NULL) {
		fprintf(stderr, "PWSOCKET not set\n");
		return 1;
	}
	// See if already running
	testFd = connect_local(pwsocket, 1);
	if (testFd != -1) {
		close(testFd);
		// Already running
		return 0;
	}
	// Prepeare vars
	if (setup_vars(envuser, envpass) == -1) {
		fprintf(stderr, "Error setting up variables\n");
		return 1;
	}
	while (*envpass) {
		*envpass++ = ' ';
	}
	// Drop privs
	setgroups(1, &gidNumber);
	if (setregid(gidNumber, gidNumber) == -1) {
		fprintf(stderr, "Warn: Could not switch to group 'nogroup'\n");
	}
	if (setreuid(uidNumber, uidNumber) == -1) {
		fprintf(stderr, "Warn: Could not switch to user %d\n", (int)uidNumber);
	}
	// Only bail out if we're not running as the user requested
	setuid(0);
	setgid(0);
	if (getuid() != uidNumber || geteuid() != uidNumber) {
		fprintf(stderr, "Fatal: Currently running as user %d\n", (int)getuid());
		return 1;
	}
	if (getgid() == 0 || getegid() == 0) {
		fprintf(stderr, "Fatal: Current process gid is 0 (root)\n");
		return 1;
	}
	// Create unix socket
	listenFd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (listenFd == -1) {
		perror("Could not create unix socket");
		return 1;
	}
	// Change permissions before bind, so it will be created with
	// the right ones right away
	if (fchmod(listenFd, S_IRUSR | S_IWUSR) == -1) {
		perror("Cannot set permissions on socket fd prior to binding");
		return 1;
	}
	remove(pwsocket);
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, pwsocket, sizeof(addr.sun_path)-1);
	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("Could not bind unix socket");
		return 1;
	}
	if (listen(listenFd, 10) == -1) {
		perror("Cannot listen on unix socket");
		return 1;
	}
	// Daemon
	daemon( 0, 0 );
	// Mainloop
	sig.sa_handler = &sig_handler;
	sigaction(SIGCHLD, &sig, NULL);
	for (;;) {
		struct sockaddr_un remote;
		socklen_t len = sizeof(remote);
		int fd = accept(listenFd, (struct sockaddr*)&remote, &len);
		if (fd != -1) {
			if (udpPort == -1) {
				udpPort = init_udp();
			}
			// Success, handle client
			pid_t child = fork();
			if (child == 0) {
				// This is the child
				struct ucred ucred;
				ssize_t ret;
				char buffer[200];
				len = sizeof(ucred);
				if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
					const char *msg = "Could not get credentials of connection\n";
					write(fd, msg, strlen(msg));
				} else if (ucred.uid != geteuid()) {
					const char *msg = "uid mismatch\n";
					write(fd, msg, strlen(msg));
				} else {
					ret = read(fd, buffer, sizeof(buffer));
					if (ret >= 3 && strncmp(buffer, "GET", 3) == 0) {
						snprintf(buffer, sizeof(buffer), "%d\t%s\t%s\t%s\n", udpPort, key1s, key2s, username);
						ret = write(fd, buffer, strlen(buffer));
					} else if (ret >= 2 && strncmp(buffer, "PW", 2) == 0) {
						int len = passwordLen - 2;
						if (len > sizeof(buffer)) {
							len = sizeof(buffer);
						}
						for (int i = 0; i < len; ++i) {
							buffer[i] = passwordEnc[i+2] ^ key2[i % KEYLEN];
						}
						ret = write(fd, buffer, len);
					}
				}
				close(fd);
				return 0;
			} else {
				// Parent, close child fd
				close(fd);
			}
		} else {
			// Error?
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ECONNABORTED)
				continue;
			perror("Fatal accept error, bailing out");
			return 1;
		}
	}
	return 0;
}

static void sig_handler(int sig)
{
	pid_t p;
	int status = sig; // Mute unused warning
	while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
		if (p == udpPid) {
			fprintf(stderr, "UDP listener died!\n");
			exit(1);
		}
	}
}

static int setup_vars(const char *envuser, const char *envpass)
{
	srand((unsigned int)getpid() ^ (unsigned int)time(NULL));
	key1 = keygen();
	key2 = keygen();
	key1s = bin2hex(key1, (size_t)KEYLEN);
	key2s = bin2hex(key2, (size_t)KEYLEN);
	username = strdup(envuser);
	passwordEnc = xorString(envpass, key2);
	passwordLen = strlen(envpass) + 2; // +2 for 2byte length prefix
	if (key1s == NULL || key2s == NULL || username == NULL || passwordEnc == NULL) {
		return -1;
	}
	return 0;
}

static uint8_t* keygen()
{
	ssize_t done = 0, ret;
	uint8_t *key = malloc(KEYLEN);
	int entropy;
	int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
	if (fd != -1) {
		if (ioctl(fd, RNDGETENTCNT, &entropy) == 0 && entropy > 0) { //Make sure we opened a random device
			while (done < KEYLEN) {
				ret = read(fd, key + done, (size_t)(KEYLEN - done));
				if (ret == -1) {
					if (errno == EINTR)
						continue;
					break;
				}
				if (ret == 0)
					break;
				done += ret;
			}
		}
		close(fd);
	}
	while (done < KEYLEN) {
		key[done++] = (char)(rand() & 0xff);
	}
	return key;
}

static uint8_t* xorString(const char* inputText, const uint8_t* key)
{
	uint8_t *text = (uint8_t*)inputText;
	size_t len = strlen(inputText);
	size_t i;
	uint8_t *retval = malloc(len + 2);
	uint8_t *ptr = retval + 2;
	retval[0] = (uint8_t)(len & 0xff00) >> 8;
	retval[1] = (uint8_t)(len & 0xff);
	for (i = 0; i < len; ++i) {
		ptr[i] = text[i] ^ key[i % KEYLEN];
	}
	return retval;
}

static char* bin2hex(uint8_t* bin, size_t len)
{
	static const char hexconvtab[] = "0123456789abcdef";
	char *retval = malloc(len * 2 + 1);
	size_t i;
	for (i = 0; i < len; ++i) {
		retval[i*2] = hexconvtab[bin[i] >> 4];
		retval[i*2+1] = hexconvtab[bin[i] & 0xf];
	}
	retval[i*2] = '\0';
	return retval;
}

static int init_udp()
{
	uint16_t port = 0;
	int fd;
	int tries = 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		perror("Cannot create udp socket");
		return -1;
	}
	for (;;) {
		port = (uint16_t)(40000 + rand() % 20000);
		struct sockaddr_in local;
		local.sin_family = AF_INET;
		local.sin_port = htons((uint16_t)port);
		local.sin_addr.s_addr = INADDR_ANY;
		if (bind(fd, (struct sockaddr*)&local, sizeof(local)) == -1) {
			if (++tries > 100) {
				perror("Cannot bind udp socket");
				close(fd);
				return -1;
			}
			continue;
		}
		break;
	}
	udpPid = fork();
	if (udpPid == -1) {
		perror("Forking udp listener failed");
		close(fd);
		return -1;
	}
	if (udpPid != 0) {
		close(fd);
		return port;
	}
	// Child
	prctl(PR_SET_PDEATHSIG, SIGTERM);
	for (;;) {
		struct sockaddr_in remote;
		socklen_t remoteLen = sizeof(remote);
		uint8_t buffer[KEYLEN];
		ssize_t ret = recvfrom(fd, buffer, KEYLEN, 0, (struct sockaddr*)&remote, &remoteLen);
		if (ret == KEYLEN && memcmp(key1, buffer, KEYLEN) == 0) {
			if (sendto(fd, passwordEnc, passwordLen, 0, (struct sockaddr*)&remote, sizeof(remote)) == -1) {
				perror("Could not send password to remote peer");
			}
		}
	}
}

