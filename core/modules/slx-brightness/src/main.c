#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <linux/input.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

int open_events( int ep );

bool handle_fd( int fd );

void handle_message( struct input_event *ev );

ssize_t file_get_contents( const char *filename, char *buffer, size_t len );

ssize_t file_put_contents( const char *filename, const char *buffer, size_t len );

int _min, _max, _step;
const char *_brightness_file;
bool _all;

int main( int argc, char **argv )
{
	int steps, ep, num, valid;
	struct epoll_event events[10];

	// List all devices by name
	if ( argc > 1 && strcmp( "-l", argv[1] ) == 0 ) {
		open_events( -1 );
		return 0;
	}

	// Listen to all devices
	if ( argc > 1 && strcmp( "-a", argv[1] ) == 0 ) {
		_all = true;
		argc--;
		argv++;
	}

	if ( argc < 5 ) {
		fprintf( stderr, "%s <path> <min> <max> <num_steps>\n", argv[0] );
		return 1;
	}

	// Sanity checks, calculate step size
	_brightness_file = argv[1];
	_min = atoi( argv[2] );
	_max = atoi( argv[3] );
	steps = atoi( argv[4] );
	if ( _max <= _min || steps < 1 ) {
		fprintf( stderr, "Error: min(%d) >= max(%d), or steps(%d) < 1\n", _min, _max, steps );
		return 1;
	}
	_step = ( _max - _min ) / steps;
	if ( _step < 1 ) {
		_step = 1;
	}

	ep = epoll_create( 100 );
	if ( ep == -1 ) {
		perror( "Cannot epoll_create" );
		return 1;
	}

	// Scan all input-event devices, open all that potentially have brightness keys
	// As of now, we match only one so epoll is pointless, but I only checked two
	// laptop models, so maybe there are different ways out there. Let's be prepared.
	valid = open_events( ep );
	if ( valid < 1 ) {
		fprintf( stderr, "Nothing to wait for, exiting\n" );
		return 0;
	}

	printf( "Waiting...\n" );
	while ( ( num = epoll_wait( ep, events, 10, -1 ) ) > 0
			|| ( num == -1 && errno == EINTR ) ) {
		for ( int i = 0; i < num; ++i ) {
			if ( !handle_fd( events[i].data.fd ) ) {
				// Try to remove
				if ( epoll_ctl( ep, EPOLL_CTL_DEL, events[i].data.fd, NULL ) == 0 ) {
					valid--;
					close( events[i].data.fd );
				}
			}
		}
	}
	if ( num == -1 ) {
		perror( "epoll error" );
		return 1;
	}
	fprintf( stderr, "Unexpected loop exit\n" );
	return 0;
}

/**
 * Open all /dev/input/eventX, check the name against known ones with
 * brightness controls.
 * Pass ep == -1 to just print all the device names.
 */
int open_events( int ep )
{
	int opened = 0;
	int fails = 0;
	int h, len;
	char fn[200];

	for ( int id = 0 ;; ++id ) {
		if ( snprintf( fn, sizeof(fn), "/dev/input/event%d", id ) == -1 ) {
			perror( "snprintf broken" );
			break;
		}

		// Open and see if success
		h = open( fn, O_RDONLY );
		if ( h == -1 ) {
			if ( errno == ENOENT ) {
				// Not found - potentially no more devices, but keep going for a bit, since for example
				// unplugging a USB device would leave a gap in the numbering.
				if ( ++fails < 10 )
					continue;
			} else {
				perror( "Cannot open eventX" );
			}
			break;
		}
		// Success, inspect
		if ( ep == -1 ) {
			printf( "Opened %s: ", fn );
		}
		fails = 0;
		len = ioctl( h, EVIOCGNAME( sizeof(fn) ), fn );
		if ( len <= 0 ) {
			perror( "Could not get device name" );
			close( h );
			continue;
		}
		if ( ep == -1 ) {
			printf( "%.*s\n", len, fn );
			close( h );
			continue;
		}

		if ( !_all && strcmp( "Video Bus", fn ) != 0 ) {
			printf( "Ignoring '%s'\n", fn );
			close( h );
			continue;
		}

		printf( "Listening on '%s'\n", fn );
		// Make nonblocking, add to epoll set
		fcntl( h, F_SETFL, O_NONBLOCK );
		struct epoll_event ee = {
			.events = EPOLLIN,
			.data.fd = h,
		};
		if ( epoll_ctl( ep, EPOLL_CTL_ADD, h, &ee ) == -1 ) {
			perror( "Cannot add eventX fd to epoll set" );
			close( h );
		} else {
			opened++;
		}
	}

	return opened;
}

/**
 * Read events from fd.
 */
bool handle_fd( int fd )
{
	int num;
	int done = 0;
	struct input_event buf;

	while ( ( num = read( fd, ( (char*)&buf ) + done, sizeof(buf) - done ) ) > 0
			|| ( num == -1 && errno == EINTR ) ) {
		if ( num + done == sizeof(buf) ) {
			handle_message( &buf );
			if ( done != 0 ) {
				done = 0;
				// Switch to nonblocking again
				fcntl( fd, F_SETFL, O_NONBLOCK );
			}
		} else {
			printf( "WARNING: PARTIAL READ\n" );
			// Switch to blocking so we can finish the message
			fcntl( fd, F_SETFL, 0 );
			done += num;
		}
	}
	if ( num == -1 && errno != EAGAIN ) {
		perror( "Error reading event" );
		return false;
	}
	return true;
}

/**
 * Gets passed read events in input_event struct,
 * check and handle brightness keys.
 */
void handle_message( struct input_event *ev )
{
	ssize_t len;
	int val, newval;
	char buf[100];

	//printf( "Type: %d, Key %d, Value %d\n", (int)ev->type, (int)ev->code, (int)ev->value );
	if ( ev->type != EV_KEY )
		return;
	if ( ev->value != 1 && ev->value != 2 )
		return; // Down or Repeat only
	if ( ev->code != KEY_BRIGHTNESSDOWN && ev->code != KEY_BRIGHTNESSUP )
		return; // Match our keys

	// Get current brightness
	len = file_get_contents( _brightness_file, buf, sizeof(buf) );
	if ( len <= 0 ) {
		fprintf( stderr, "Cannot read brightness\n" );
		return;
	}

	// Calculate new brightness
	val = atoi( buf );
	newval = val +  _step * ( ev->code == KEY_BRIGHTNESSUP ? 1 : -1 );
	if ( newval < _min ) {
		newval = _min;
	} else if ( newval > _max ) {
		newval = _max;
	}
	if ( val == newval )
		return;

	// Set new brightness
	len = snprintf( buf, sizeof(buf), "%d", newval );
	if ( len == -1 ) {
		perror( "Brightness snprintf fail" );
	} else if ( len == 0 ) {
		fprintf( stderr, "Brightness snprintf returned zero\n" );
	} else {
		file_put_contents( _brightness_file, buf, (size_t)len );
	}
}

/**
 * Read file contents into buffer.
 */
ssize_t file_get_contents( const char *filename, char *buffer, size_t len )
{
	ssize_t ret;
	size_t done = 0;
	ssize_t bytes_read = -1;
	int fd;

	if ( len <= 1 ) {
		if ( len == 1 ) {
			buffer[0] = '\0';
		}
		return 0;
	}
	len--;

	fd = open( filename, O_RDONLY );
	if ( fd == -1 ) {
		fprintf( stderr, "%s: ", filename );
		perror( "Error opening file" );
		return -1;
	}

	while ( done < len && ( bytes_read = read( fd, buffer + done, len - done ) ) > 0 ) {
		done += bytes_read;
	}
	if ( bytes_read == -1 ) {
		fprintf( stderr, "%s: ", filename );
		perror( "Error reading file" );
		ret = -1;
	} else {
		ret = (ssize_t)done;
		buffer[done] = '\0';
	}

	close( fd );
	return ret;
}

/**
 * Write buffer contents to file.
 */
ssize_t file_put_contents( const char *filename, const char *buffer, size_t len )
{
	ssize_t ret;
	size_t done = 0;
	ssize_t bytes_written = -1;
	int fd = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 0644 );

	if ( fd == -1 ) {
		fprintf( stderr, "%s: ", filename );
		perror( "Error opening file" );
		return -1;
	}

	while ( done < len && ( bytes_written = write( fd, buffer + done, len - done ) ) > 0 ) {
		done += bytes_written;
	}
	if ( bytes_written == -1 ) {
		fprintf( stderr, "%s: ", filename );
		perror( "Error writing file" );
		ret = -1;
	} else {
		ret = (ssize_t)done;
	}
	close( fd );
	return ret;
}
