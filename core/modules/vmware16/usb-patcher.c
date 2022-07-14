#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

char* findSequence(char *buffer, size_t len, size_t offset);

int main(int argc, char**argv)
{
	int fd;
	ssize_t flen;
	char *exe, *offset;
	fd = open( argv[1], O_RDWR );
	if ( fd == -1 ) {
		perror( "Cannot open argv[1]" );
		return 1;
	}
	flen = lseek( fd, 0, SEEK_END );
	if ( flen < 0 ) {
		perror( "File length bad" );
		return 2;
	}
	flen &= ~( sysconf( _SC_PAGE_SIZE ) - 1 );
	printf( "Mapping %d bytes.\n", (int)flen );
	exe = mmap( NULL, flen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
	if ( exe == NULL ) {
		perror( "mmap failed" );
		return 3;
	}
	close( fd );
	offset = findSequence( exe, flen, 0 );
	if ( offset == NULL ) {
		printf( "Sequence not found\n" );
		return 4;
	}
	if ( findSequence( exe, flen, ( offset - exe ) + 19 ) != NULL ) {
		printf( "Found more than one match\n" );
		return 5;
	}
	printf( "Mapped at %p, instruction at %p\n", exe, offset );
	offset[8] = '\xfd';
	munmap( exe, flen );
	printf( "File patched.\n" );
	return 0;
}


char* findSequence(char *buffer, size_t len, size_t offset)
{
	return memmem( buffer + offset, len - offset,
			"\x41\x56\x41\x55\x41\x54\x41\x89\xf5\x55\x53\x48\x89\xfd\xbf\x0d\x00\x00\x00", 19 );
}
