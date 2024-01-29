#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		puts("Nee\n");
		return 1;
	}
	char * const nargv[] = {
		"bash",
		"/opt/openslx/vmchooser/scripts/set-firewall",
		argv[1],
		0
	};
	char * const nenv[] = {
		"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/opt/openslx/sbin:/opt/openslx/bin",
		"HOME=/root",
		"LC_ALL=C.UTF-8",
		"LANG=C.UTF-8",
		0
	};

	setresuid(0, 0, 0);
	setregid(0, 0);

	execve("/bin/bash", nargv, nenv);
}

