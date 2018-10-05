/* xprintlocktime
 *
 * Print unix timestamp of then the xscreensaver went into locked state
 * to stdout and return with exit code 0
 * If xscreensaver is not currently active in locked state, an error message
 * will be printed to stderr and the exit code is nonzero.
 *
 * Based on xscreensaver-command, original copyright notice follows:
 *
 * xscreensaver-command, Copyright (c) 1991-2009 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

static Window find_screensaver_window( Display *dpy, char **version );

Atom XA_SCREENSAVER_VERSION, XA_SCREENSAVER_STATUS, XA_LOCK;

static XErrorHandler old_handler = 0;
static Bool got_badwindow = False;
static int BadWindow_ehandler( Display *dpy, XErrorEvent *error )
{
    if( error->error_code == BadWindow ) {
        got_badwindow = True;
        return 0;
    } else {
        if( !old_handler ) {
            abort();
        }
        return ( *old_handler )( dpy, error );
    }
}

int main()
{
    Display *dpy = XOpenDisplay( getenv( "DISPLAY" ) );
    XWindowAttributes xgwa;
    if( dpy == NULL ) {
        fprintf( stderr, "Cannot open display\n" );
        return 1;
    }

    XA_SCREENSAVER_STATUS = XInternAtom( dpy, "_SCREENSAVER_STATUS", False );
    XA_SCREENSAVER_VERSION = XInternAtom( dpy, "_SCREENSAVER_VERSION",False );
    XA_LOCK = XInternAtom( dpy, "LOCK", False );


    char *v = NULL;
    Window window = find_screensaver_window( dpy, &v );
    if( !window ) {
        fprintf( stderr, "no screensaver running\n" );
        return 1;
    }
    if( !v || !*v ) {
        fprintf( stderr, "version property not set on window 0x%x?\n",
                 ( unsigned int ) window );
        return 1;
    }

    /* Select for property change events, so that we can read the response. */
    XGetWindowAttributes( dpy, window, &xgwa );
    XSelectInput( dpy, window, xgwa.your_event_mask | PropertyChangeMask );

    XClassHint hint;
    memset( &hint, 0, sizeof( hint ) );
    XGetClassHint( dpy, window, &hint );
    if( !hint.res_class ) {
        fprintf( stderr, "class hints not set on window 0x%x?\n",
                 ( unsigned int ) window );
        return 1;
    }

    Atom type;
    int format;
    unsigned long nitems, bytesafter;
    unsigned char *dataP = 0;

    if( XGetWindowProperty( dpy,
                            RootWindow( dpy, 0 ),
                            XA_SCREENSAVER_STATUS,
                            0, 999, False, XA_INTEGER,
                            &type, &format, &nitems, &bytesafter,
                            &dataP )
            != Success
            || !type
            || !dataP ) {
        fprintf( stderr, "Foof foof! No property\n" );
        return 1;
    }
    Atom *data = ( Atom * ) dataP;
    if( type != XA_INTEGER || nitems < 3 ) {
        if( data ) {
            free( data );
        }
        fprintf( stdout, "\n" );
        fflush( stdout );
        fprintf( stderr, "bad status format on root window.\n" );
        return 1;
    }

    Atom blanked = ( Atom ) data[0];
    time_t tt = ( time_t ) data[1];

    if( tt <= ( time_t ) 666000000L ) { /* early 1991 */
        fprintf( stderr, "Bad lock time reported\n" );
        return 1;
    }

    if( blanked == XA_LOCK ) {
        printf( "%llu\n", ( unsigned long long )tt );
        return 0;
    }
    fprintf( stderr, "Not locked\n" );
    return 1;
}

static Window find_screensaver_window( Display *dpy, char **version )
{
    int i;
    Window root = RootWindowOfScreen( DefaultScreenOfDisplay( dpy ) );
    Window root2, parent, *kids;
    unsigned int nkids;

    if( version ) {
        *version = 0;
    }

    if( ! XQueryTree( dpy, root, &root2, &parent, &kids, &nkids ) ) {
        abort();
    }
    if( root != root2 ) {
        abort();
    }
    if( parent ) {
        abort();
    }
    if( !( kids && nkids ) ) {
        return 0;
    }
    for( i = 0; i < nkids; i++ ) {
        Atom type;
        int format;
        unsigned long nitems, bytesafter;
        unsigned char *v;
        int status;

        /* We're walking the list of root-level windows and trying to find
           the one that has a particular property on it.  We need to trap
           BadWindows errors while doing this, because it's possible that
           some random window might get deleted in the meantime.  (That
           window won't have been the one we're looking for.)
         */
        XSync( dpy, False );
        if( old_handler ) {
            abort();
        }
        got_badwindow = False;
        old_handler = XSetErrorHandler( BadWindow_ehandler );
        status = XGetWindowProperty( dpy, kids[i],
                                     XA_SCREENSAVER_VERSION,
                                     0, 200, False, XA_STRING,
                                     &type, &format, &nitems, &bytesafter,
                                     &v );
        XSync( dpy, False );
        XSetErrorHandler( old_handler );
        old_handler = 0;

        if( got_badwindow ) {
            status = BadWindow;
            got_badwindow = False;
        }

        if( status == Success && type != None ) {
            Window ret = kids[i];
            if( version ) {
                *version = ( char* )v;
            }
            XFree( kids );
            return ret;
        }
    }

    if( kids ) {
        XFree( kids );
    }
    return 0;
}

