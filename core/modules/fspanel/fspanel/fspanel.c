
/********************************************************
 ** F***ing Small Panel 0.7 Copyright (c) 2000-2001 By **
 ** Peter Zelezny <zed@linuxpower.org>                 **
 ** See file COPYING for license details.              **
 ********************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_XPM
#include <X11/xpm.h>
#include "icon.xpm"
#endif

#include "fspanel.h"

/* you can edit these */
#define MAX_TASK_WIDTH 145
#define ICONWIDTH 16
#define ICONHEIGHT 16
#define WINHEIGHT 24
#define WINWIDTH (scr_width)
#define FONT_NAME "-*-lucida*-m*-r-*-*-12-*-*"

/* don't edit these */
#define TEXTPAD 6
#define left_arrow_x 25
#define right_arrow_x 50

Display *dd;
Window root_win;
/* WM supports EWMH
This flag is set if the window manager supports the EWMH protocol for e.g.
switching workspaces. The fallback if this is not supported is to use the
Gnome variant. This is determined by looking for the presence of the
_NET_SUPPORTED property of the root window. Note that this is only used
for communication with the WM, whether each client supports this protocol
is up to the individual client. */
int wm_use_ewmh;
int always_on_top = 1;
Pixmap generic_icon;
Pixmap generic_mask;
GC fore_gc;
XFontStruct *xfs;
int scr_screen;
int scr_depth;
int scr_width;
int scr_height;
int text_y;
/*int time_width;*/

unsigned short cols[] = {
	0xd75c, 0xd75c, 0xd75c,		  /* 0. light gray */
	0xbefb, 0xbaea, 0xbefb,		  /* 1. mid gray */
	0xaefb, 0xaaea, 0xaefb,		  /* 2. dark gray */
	0xefbe, 0xefbe, 0xefbe,		  /* 3. white */
	0x8617, 0x8207, 0x8617,		  /* 4. darkest gray */
	0x0000, 0x0000, 0x0000		  /* 5. black */
};

#define PALETTE_COUNT (sizeof (cols) / sizeof (cols[0]) / 3)

unsigned long palette[PALETTE_COUNT];

char *atom_names[] = {
	"KWM_WIN_ICON",
	"_MOTIF_WM_HINTS",
	"_WIN_WORKSPACE",
	"_WIN_HINTS",
	"_WIN_LAYER",
	"_NET_CLIENT_LIST",
	"_WIN_CLIENT_LIST",
	"_WIN_WORKSPACE_COUNT",
	"_WIN_STATE",
	"WM_STATE",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_CURRENT_DESKTOP",
	"_NET_WM_STATE",
	"_NET_WM_STATE_ABOVE",
	"_NET_SUPPORTED",
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DESKTOP",
	"_NET_WM_DESKTOP",
	"_NET_WM_NAME",
	"UTF8_STRING",
};

#define ATOM_COUNT (sizeof (atom_names) / sizeof (atom_names[0]))

Atom atoms[ATOM_COUNT];

#define atom_KWM_WIN_ICON atoms[0]
#define atom__MOTIF_WM_HINTS atoms[1]
#define atom__WIN_WORKSPACE atoms[2]
#define atom__WIN_HINTS atoms[3]
#define atom__WIN_LAYER atoms[4]
#define atom__NET_CLIENT_LIST atoms[5]
#define atom__WIN_CLIENT_LIST atoms[6]
#define atom__WIN_WORKSPACE_COUNT atoms[7]
#define atom__WIN_STATE atoms[8]
#define atom_WM_STATE atoms[9]
#define atom__NET_NUMBER_OF_DESKTOPS atoms[10]
#define atom__NET_CURRENT_DESKTOP atoms[11]
#define atom__NET_WM_STATE atoms[12]
#define atom__NET_WM_STATE_ABOVE atoms[13]
#define atom__NET_SUPPORTED atoms[14]
#define atom__NET_WM_WINDOW_TYPE atoms[15]
#define atom__NET_WM_WINDOW_TYPE_DESKTOP atoms[16]
#define atom__NET_WM_DESKTOP atoms[17]
#define atom__NET_WM_NAME atoms[18]
#define atom_UTF8_STRING atoms[19]


void *
get_prop_data (Window win, Atom prop, Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty (dd, win, prop, 0, 0x7fffffff, False,
							  type, &type_ret, &format_ret, &items_ret,
							  &after_ret, &prop_data);
	if (items)
		*items = items_ret;

	return prop_data;
}

void
set_foreground (int index)
{
	XSetForeground (dd, fore_gc, palette[index]);
}

void
draw_line (taskbar *tb, int x, int y, int a, int b)
{
	XDrawLine (dd, tb->win, fore_gc, x, y, a, b);
}

void
fill_rect (taskbar *tb, int x, int y, int a, int b)
{
	XFillRectangle (dd, tb->win, fore_gc, x, y, a, b);
}

void
scale_icon (task *tk)
{
	int xx, yy, x, y;
	unsigned int w, h, d, bw;
	Pixmap pix, mk = None;
	XGCValues gcv;
	GC mgc = None;

	Window unused;
	XGetGeometry (dd, tk->icon, &unused, &x, &y, &w, &h, &bw, &d);
	pix = XCreatePixmap (dd, tk->win, ICONWIDTH, ICONHEIGHT, scr_depth);

	if (tk->mask != None)
	{
		mk = XCreatePixmap (dd, tk->win, ICONWIDTH, ICONHEIGHT, 1);
		gcv.subwindow_mode = IncludeInferiors;
		gcv.graphics_exposures = False;
		mgc = XCreateGC (dd, mk, GCGraphicsExposures | GCSubwindowMode, &gcv);
	}

	set_foreground (3);

	/* this is my simple & dirty scaling routine */
	for (y = ICONHEIGHT - 1; y >= 0; y--)
	{
		yy = (y * h) / ICONHEIGHT;
		for (x = ICONWIDTH - 1; x >= 0; x--)
		{
			xx = (x * w) / ICONWIDTH;
			if (d != scr_depth)
				XCopyPlane (dd, tk->icon, pix, fore_gc, xx, yy, 1, 1, x, y, 1);
			else
				XCopyArea (dd, tk->icon, pix, fore_gc, xx, yy, 1, 1, x, y);
			if (mk != None)
				XCopyArea (dd, tk->mask, mk, mgc, xx, yy, 1, 1, x, y);
		}
	}

	if (mk != None)
	{
		XFreeGC (dd, mgc);
		tk->mask = mk;
	}

	tk->icon = pix;
}

void
get_task_hinticon (task *tk)
{
	XWMHints *hin;

	tk->icon = None;
	tk->mask = None;

	hin = (XWMHints *) get_prop_data (tk->win, XA_WM_HINTS, XA_WM_HINTS, 0);
	if (hin)
	{
		if ((hin->flags & IconPixmapHint))
		{
			if ((hin->flags & IconMaskHint))
			{
				tk->mask = hin->icon_mask;
			}

			tk->icon = hin->icon_pixmap;
			tk->icon_copied = 1;
			scale_icon (tk);
		}
		XFree (hin);
	}

	if (tk->icon == None)
	{
		tk->icon = generic_icon;
		tk->mask = generic_mask;
	}
}

void
get_task_kdeicon (task *tk)
{
	unsigned long *data;

	data = get_prop_data (tk->win, atom_KWM_WIN_ICON, atom_KWM_WIN_ICON, 0);
	if (data)
	{
		tk->icon = data[0];
		tk->mask = data[1];
		XFree (data);
	}
}

/* returns whether the window is visible on the desktop */
int
is_visible_on_desktop (Window win, int desk)
{
	int client_desk = -1;
	unsigned long *data;

	if (wm_use_ewmh)
		data = get_prop_data (win, atom__NET_WM_DESKTOP, XA_CARDINAL, 0);
	else
		data = get_prop_data (win, atom__WIN_WORKSPACE, XA_CARDINAL, 0);

	if (data)
	{
		client_desk = *data;
		XFree (data);
	}

	/* If the client_desk is -1, it is visible on all desktops */
	return (client_desk == -1) || (client_desk == desk);
}

/* index of the currently displayed desktop */
int
get_current_desktop ()
{
	int desk = 0;
	unsigned long *data;

	if (wm_use_ewmh)
		data = get_prop_data (root_win, atom__NET_CURRENT_DESKTOP, XA_CARDINAL, 0);
	else
		data = get_prop_data (root_win, atom__WIN_WORKSPACE, XA_CARDINAL, 0);

	if (data)
	{
		desk = *data;
		XFree (data);
	}
	return desk;
}

int
is_hidden (Window win)
{
	unsigned long *data;
	int ret = 0;

	data = get_prop_data (win, atom__WIN_HINTS, XA_CARDINAL, 0);
	if (data)
	{
		if ((*data) & WIN_HINTS_SKIP_TASKBAR)
			ret = 1;
		XFree (data);
	}

	return ret;
}

int
is_iconified (Window win)
{
	unsigned long *data;
	int ret = 0;

	data = get_prop_data (win, atom_WM_STATE, atom_WM_STATE, 0);
	if (data)
	{
		if (data[0] == IconicState)
			ret = 1;
		XFree (data);
	}
	return ret;
}

/* window name
The returned pointer must be freed using XFree() after use.

TODO: The encoding for WM_NAME can be STRING or COMPOUND_TEXT. In any case
this encoding should be normalized before returning from this function. */
char*
get_window_name (Window win)
{
	char* res = NULL;
	/* try EWMH's _NET_WM_NAME first */
	res = get_prop_data (win, atom__NET_WM_NAME, atom_UTF8_STRING, 0);
	if (!res)
		/* fallback to WM_NAME */
		res = get_prop_data (win, XA_WM_NAME, XA_STRING, 0);
	return res;
}

void
add_task (taskbar * tb, Window win, int focus)
{
	task *tk, *list;

	/* is this window on a different desktop or hidden? */
	if (!is_visible_on_desktop (win, tb->my_desktop) || is_hidden (win))
		return;

	tk = calloc (1, sizeof (task));
	tk->win = win;
	tk->focused = focus;
	tk->name = get_window_name (win);
	tk->iconified = is_iconified (win);

	get_task_kdeicon (tk);
	if (tk->icon == None)
		get_task_hinticon (tk);

	XSelectInput (dd, win, PropertyChangeMask | FocusChangeMask |
					  StructureNotifyMask);

	/* now append it to our linked list */
	tb->num_tasks++;

	list = tb->task_list;
	if (!list)
	{
		tb->task_list = tk;
		return;
	}
	while (1)
	{
		if (!list->next)
		{
			list->next = tk;
			return;
		}
		list = list->next;
	}
}

void
gui_sync (void)
{
	XSync (dd, False);
}

void
set_prop (Window win, Atom at, long val)
{
	XChangeProperty (dd, win, at, XA_CARDINAL, 32,
						  PropModeReplace, (unsigned char *) &val, 1);
}

taskbar *
gui_create_taskbar (void)
{
	taskbar *tb;
	Window win;
	XClassHint wm_class;
	MWMHints mwm;
	XSizeHints size_hints;
	XWMHints wmhints;
	XSetWindowAttributes att;

	att.background_pixel = palette[0];
	att.event_mask = ButtonPressMask | ExposureMask;

	win = XCreateWindow (
								  /* display */ dd,
								  /* parent  */ root_win,
								  /* x       */ 0,
								  /* y       */ scr_height - WINHEIGHT,
								  /* width   */ WINWIDTH,
								  /* height  */ WINHEIGHT,
								  /* border  */ 0,
								  /* depth   */ CopyFromParent,
								  /* class   */ InputOutput,
								  /* visual  */ CopyFromParent,
								  /*value mask*/ CWBackPixel | CWEventMask,
								  /* attribs */ &att);

	/* set name and class */
	wm_class.res_name = "fspanel";
	wm_class.res_class = "fspanel";
	XSetClassHint (dd, win, &wm_class);

	/* don't let any windows cover fspanel */
	if (wm_use_ewmh)
	{
		if (always_on_top)
		{
			XChangeProperty (dd, win,
			                 atom__NET_WM_STATE, XA_ATOM, 32,
			                 PropModeReplace,
			                 (unsigned char *) &atom__NET_WM_STATE_ABOVE, 1);
		}

		XChangeProperty (dd, win,
		                 atom__NET_WM_WINDOW_TYPE, XA_ATOM, 32,
		                 PropModeReplace,
		                 (unsigned char *) &atom__NET_WM_WINDOW_TYPE_DESKTOP, 1);
	}
	else
	{
		set_prop (win, atom__WIN_LAYER, 10);	/* WIN_LAYER_ABOVE_DOCK */
		set_prop (win, atom__WIN_STATE,
		          WIN_STATE_STICKY | WIN_STATE_FIXED_POSITION);
	}

	set_prop (win, atom__WIN_HINTS, WIN_HINTS_SKIP_FOCUS |
				 WIN_HINTS_SKIP_WINLIST |
				 WIN_HINTS_SKIP_TASKBAR | WIN_HINTS_DO_NOT_COVER);

	/* borderless motif hint */
	memset (&mwm, 0, sizeof (mwm));
	mwm.flags = MWM_HINTS_DECORATIONS;
	XChangeProperty (dd, win, atom__MOTIF_WM_HINTS,
							atom__MOTIF_WM_HINTS, 32, PropModeReplace,
							(unsigned char *) &mwm, sizeof (MWMHints) / 4);

	/* make sure the WM obeys our window position */
	size_hints.flags = PPosition;
	/*XSetWMNormalHints (dd, win, &size_hints);*/
	XChangeProperty (dd, win, XA_WM_NORMAL_HINTS,
							XA_WM_SIZE_HINTS, 32, PropModeReplace,
							(unsigned char *) &size_hints, sizeof (XSizeHints) / 4);

	/* make our window unfocusable */
	wmhints.flags = InputHint;
	wmhints.input = 0;
	/*XSetWMHints (dd, win, &wmhints);*/
	XChangeProperty (dd, win, XA_WM_HINTS,
							XA_WM_HINTS, 32, PropModeReplace,
							(unsigned char *) &wmhints, sizeof (XWMHints) / 4);

	XMapWindow (dd, win);

	tb = calloc (1, sizeof (taskbar));
	tb->win = win;

	return tb;
}

void
gui_init (void)
{
	XGCValues gcv;
	XColor xcl;
	int i, j;
	char *fontname;

	i = j = 0;
	do
	{
		xcl.red = cols[i];
		i++;
		xcl.green = cols[i];
		i++;
		xcl.blue = cols[i];
		i++;
		XAllocColor (dd, DefaultColormap (dd, scr_screen), &xcl);
		palette[j] = xcl.pixel;
		j++;
	}
	while (j < PALETTE_COUNT);

	fontname = FONT_NAME;
	do
	{
		xfs = XLoadQueryFont (dd, fontname);
		fontname = "fixed";
	}
	while (!xfs);

	/*time_width = XTextWidth (xfs, "88:88", 5); */
#define time_width (35)
	text_y = xfs->ascent + ((WINHEIGHT - xfs->ascent) / 2);

	gcv.font = xfs->fid;
	gcv.graphics_exposures = False;
	fore_gc = XCreateGC (dd, root_win, GCFont | GCGraphicsExposures, &gcv);

#ifdef HAVE_XPM
	XpmCreatePixmapFromData (dd, root_win, icon_xpm, &generic_icon,
									 &generic_mask, NULL);
#else
	generic_icon = 0;
#endif
}

void
gui_draw_vline (taskbar * tb, int x)
{
	set_foreground (4);
	draw_line (tb, x, 0, x, WINHEIGHT);
	set_foreground (3);
	draw_line (tb, x + 1, 0, x + 1, WINHEIGHT);
}

void
gui_draw_task (taskbar * tb, task * tk)
{
	int len;
	int x = tk->pos_x;
	int taskw = tk->width;

	if (!tk->name)
		return;

	gui_draw_vline (tb, x);

	/*set_foreground (3); *//* it's already 3 from gui_draw_vline() */
	draw_line (tb, x + 1, 0, x + taskw, 0);

	set_foreground (1);
	draw_line (tb, x + 1, WINHEIGHT - 1, x + taskw, WINHEIGHT - 1);

	if (tk->focused)
	{
		x++;
		/*set_foreground (1);*/		  /* mid gray */
		fill_rect (tb, x + 3, 3, taskw - 5, WINHEIGHT - 6);
		set_foreground (3);		  /* white */
		draw_line (tb, x + 2, WINHEIGHT - 2, x + taskw - 2, WINHEIGHT - 2);
		draw_line (tb, x + taskw - 2, 2, x + taskw - 2, WINHEIGHT - 2);
		set_foreground (0);
		draw_line (tb, x + 1, 2, x + 1, WINHEIGHT - 2);
		set_foreground (4);		  /* darkest gray */
		draw_line (tb, x + 2, 2, x + taskw - 2, 2);
		draw_line (tb, x + 2, 2, x + 2, WINHEIGHT - 3);
	} else
	{
		set_foreground (0);		  /* mid gray */
		fill_rect (tb, x + 2, 1, taskw - 1, WINHEIGHT - 2);
	}

	{
		register int text_x = x + TEXTPAD + TEXTPAD + ICONWIDTH;

		/* check how many chars can fit */
		len = strlen (tk->name);
		while (XTextWidth (xfs, tk->name, len) >= taskw - (text_x - x) - 2
				 && len > 0)
			len--;

		if (tk->iconified)
		{
			/* draw task's name dark (iconified) */
			set_foreground (3);
			XDrawString (dd, tb->win, fore_gc, text_x, text_y + 1, tk->name,
							 len);
			set_foreground (4);
		} else
		{
			set_foreground (5);
		}

		/* draw task's name here */
		XDrawString (dd, tb->win, fore_gc, text_x, text_y, tk->name, len);
	}

#ifndef HAVE_XPM
	if (!tk->icon)
		return;
#endif

	/* draw the task's icon */
	XSetClipMask (dd, fore_gc, tk->mask);
	XSetClipOrigin (dd, fore_gc, x + TEXTPAD, (WINHEIGHT - ICONHEIGHT) / 2);
	XCopyArea (dd, tk->icon, tb->win, fore_gc, 0, 0, ICONWIDTH, ICONHEIGHT,
				  x + TEXTPAD, (WINHEIGHT - ICONHEIGHT) / 2);
	XSetClipMask (dd, fore_gc, None);
}

void
gui_draw_clock (taskbar * tb)
{
	char *time_str;
	time_t now;
	int width, old_x, x = WINWIDTH - time_width - (TEXTPAD * 4);

	old_x = x;

	width = WINWIDTH - x - 2;

	now = time (0);
	time_str = ctime (&now) + 11;

	gui_draw_vline (tb, x);
	x += TEXTPAD;

	/*set_foreground (3); *//* white *//* it's already 3 from gui_draw_vline() */
	draw_line (tb, x + 1, WINHEIGHT - 2, old_x + width - TEXTPAD,
				  WINHEIGHT - 2);
	draw_line (tb, old_x + width - TEXTPAD, 2, old_x + width - TEXTPAD,
				  WINHEIGHT - 2);

	set_foreground (1);			  /* mid gray */
	fill_rect (tb, x + 1, 2, width - (TEXTPAD * 2) - 1, WINHEIGHT - 4);

	set_foreground (4);			  /* darkest gray */
	draw_line (tb, x, 2, x + width - (TEXTPAD * 2) - 1, 2);
	draw_line (tb, x, 2, x, WINHEIGHT - 2);

	set_foreground (5);
	XDrawString (dd, tb->win, fore_gc, x + TEXTPAD - 1, text_y, 
						time_str, 5);
}

void
draw_dot (Window win, int x, int y)
{
	set_foreground (4);
	XDrawPoint (dd, win, fore_gc, x, y);
	set_foreground (3);
	XDrawPoint (dd, win, fore_gc, x + 1, y + 1);
}

void
draw_grill (Window win, int x)
{
	int y = 0;
	while (y < WINHEIGHT - 4)
	{
		y += 3;
		draw_dot (win, x + 3, y);
		draw_dot (win, x, y);
	}
}

void
draw_up_triangle (taskbar *tb)
{
	fill_rect (tb, left_arrow_x + 2, (WINHEIGHT - 4) / 2 + 1, 3, 3);
	draw_line (tb, left_arrow_x, (WINHEIGHT - 4) / 2 + 3, left_arrow_x + 3, (WINHEIGHT - 4) / 2);
	draw_line (tb, left_arrow_x + 3, (WINHEIGHT - 4) / 2, left_arrow_x + 6, (WINHEIGHT - 4) / 2 + 3);
	draw_line (tb, left_arrow_x + 1, (WINHEIGHT - 4) / 2 + 3, left_arrow_x + 5, (WINHEIGHT - 4) / 2 + 3);
}

void
draw_down_triangle (taskbar *tb)
{
	draw_line (tb, right_arrow_x, (WINHEIGHT - 4) / 2, right_arrow_x + 3, (WINHEIGHT - 4) / 2 + 3);
	draw_line (tb, right_arrow_x, (WINHEIGHT - 4) / 2, right_arrow_x + 4, (WINHEIGHT - 4) / 2 + 2);
	draw_line (tb, right_arrow_x, (WINHEIGHT - 4) / 2, right_arrow_x + 5, (WINHEIGHT - 4) / 2 + 1);
	draw_line (tb, right_arrow_x, (WINHEIGHT - 4) / 2, right_arrow_x + 6, (WINHEIGHT - 4) / 2);
}

void
gui_draw_taskbar (taskbar * tb)
{
	task *tk;
	int x, width, taskw;
	int under = 0;

	set_foreground (5);	/* black */
	draw_up_triangle (tb);
	draw_down_triangle (tb);

	width = WINWIDTH - 80 - time_width - (TEXTPAD * 4);
	x = 80;

	if (tb->num_tasks == 0)
		goto clear;

	taskw = width / tb->num_tasks;
	if (taskw > MAX_TASK_WIDTH)
	{
		taskw = MAX_TASK_WIDTH;
		under = 1;
	}

	tk = tb->task_list;
	while (tk)
	{
		tk->pos_x = x;
		tk->width = taskw - 1;
		gui_draw_task (tb, tk);
		x += taskw;
		tk = tk->next;
	}

	if (under)
	{
clear:
		gui_draw_vline (tb, x);
		set_foreground (0);
		fill_rect (tb, x + 2, 0, WINWIDTH, WINHEIGHT);
	}

	gui_draw_clock (tb);

	gui_draw_vline (tb, 8);
	gui_draw_vline (tb, 74);

	draw_grill (tb->win, 2);
	draw_grill (tb->win, WINWIDTH - 6);
}

task *
find_task (taskbar * tb, Window win)
{
	task *list = tb->task_list;
	while (list)
	{
		if (list->win == win)
			return list;
		list = list->next;
	}
	return 0;
}

void
del_task (taskbar * tb, Window win)
{
	task *next, *prev = 0, *list = tb->task_list;

	while (list)
	{
		next = list->next;
		if (list->win == win)
		{
			/* unlink and free this task */
			tb->num_tasks--;
			if (list->icon_copied)
			{
				XFreePixmap (dd, list->icon);
				if (list->mask != None)
					XFreePixmap (dd, list->mask);
			}
			if (list->name)
				XFree (list->name);
			free (list);
			if (prev == 0)
				tb->task_list = next;
			else
				prev->next = next;
			return;
		}
		prev = list;
		list = next;
	}
}

void
taskbar_read_clientlist (taskbar * tb)
{
	Window *win, focus_win;
	int num, i, rev, desk, new_desk = 0;
	task *list, *next;

	desk = get_current_desktop ();
	if (desk != tb->my_desktop)
	{
		new_desk = 1;
		tb->my_desktop = desk;
	}

	XGetInputFocus (dd, &focus_win, &rev);

	/* try unified window spec first */
	win = get_prop_data (root_win, atom__NET_CLIENT_LIST, XA_WINDOW, &num);
	if (!win)
	{
		/* failed, let's try gnome */
		win = get_prop_data (root_win, atom__WIN_CLIENT_LIST, XA_CARDINAL, &num);
		if (!win)
			return;
	}

	/* remove windows that aren't in the _WIN_CLIENT_LIST anymore */
	list = tb->task_list;
	while (list)
	{
		list->focused = (focus_win == list->win);
		next = list->next;

		if (!new_desk)
			for (i = num - 1; i >= 0; i--)
				if (list->win == win[i])
					goto dontdel;
		del_task (tb, list->win);
dontdel:

		list = next;
	}

	/* add any new windows */
	for (i = 0; i < num; i++)
	{
		if (!find_task (tb, win[i]))
			add_task (tb, win[i], (win[i] == focus_win));
	}

	XFree (win);
}

void
move_taskbar (taskbar * tb)
{
	int x, y;

	x = y = 0;

	if (tb->hidden)
		x = WINWIDTH - TEXTPAD;

	if (!tb->at_top)
		y = scr_height - WINHEIGHT;

	XMoveWindow (dd, tb->win, x, y);
}

void
switch_desk (taskbar * tb, int rel)
{
	XClientMessageEvent xev;
	unsigned long *data, max_desks;
	int want = tb->my_desktop + rel;

	if (want < 0)
		return;

	if (wm_use_ewmh)
		data = get_prop_data (root_win, atom__NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 0);
	else
		data = get_prop_data (root_win, atom__WIN_WORKSPACE_COUNT, XA_CARDINAL, 0);
	if (!data)
		/* number of workspaces not available */
		return;

	max_desks = *data;
	XFree (data);
	if (max_desks <= want)
		return;

	xev.type = ClientMessage;
	xev.window = root_win;
	if (wm_use_ewmh)
		xev.message_type = atom__NET_CURRENT_DESKTOP;
	else
		xev.message_type = atom__WIN_WORKSPACE;
	xev.format = 32;
	xev.data.l[0] = want;
	XSendEvent (dd, root_win, False, SubstructureNotifyMask, (XEvent *) &xev);
}

void
handle_press (taskbar * tb, int x, int y)
{
	task *tk;

	if (y > 3 && y < WINHEIGHT - 3)
	{
		if (x >= right_arrow_x && x < right_arrow_x + 9)
		{
			switch_desk (tb, +1);
			return;
		}

		if (x >= left_arrow_x && x < left_arrow_x + 9)
		{
			switch_desk (tb, -1);
			return;
		}
	}

	/* clicked left grill */
	if (x < 6)
	{
		if (tb->hidden)
			tb->hidden = 0;
		else
			tb->at_top = !tb->at_top;
		move_taskbar (tb);
		return;
	}

	/* clicked right grill */
	if (x + TEXTPAD > WINWIDTH)
	{
		tb->hidden = !tb->hidden;
		move_taskbar (tb);
		return;
	}

	tk = tb->task_list;
	while (tk)
	{
		if (x > tk->pos_x && x < tk->pos_x + tk->width)
		{
			if (tk->iconified)
			{
				tk->iconified = 0;
				tk->focused = 1;
				XMapWindow (dd, tk->win);
			} else
			{
				if (tk->focused)
				{
					tk->iconified = 1;
					tk->focused = 0;
					XIconifyWindow (dd, tk->win, scr_screen);
				} else
				{
					tk->focused = 1;
					XRaiseWindow (dd, tk->win);
					XSetInputFocus (dd, tk->win, RevertToNone, CurrentTime);
				}
			}
			gui_sync ();
			gui_draw_task (tb, tk);
		} else
		{
			if (tk->focused)
			{
				tk->focused = 0;
				gui_draw_task (tb, tk);
			}
		}

		tk = tk->next;
	}
}

void
handle_focusin (taskbar * tb, Window win)
{
	task *tk;

	tk = tb->task_list;
	while (tk)
	{
		if (tk->focused)
		{
			if (tk->win != win)
			{
				tk->focused = 0;
				gui_draw_task (tb, tk);
			}
		} else
		{
			if (tk->win == win)
			{
				tk->focused = 1;
				gui_draw_task (tb, tk);
			}
		}
		tk = tk->next;
	}
}

void
handle_propertynotify (taskbar * tb, Window win, Atom at)
{
	task *tk;

	if (win == root_win)
	{
		if (wm_use_ewmh)
		{
			if (at == atom__NET_CLIENT_LIST ||
			    at == atom__NET_CURRENT_DESKTOP)
			{
				taskbar_read_clientlist (tb);
				gui_draw_taskbar (tb);
			}
		}
		else
		{
			if (at == atom__WIN_CLIENT_LIST ||
			    at == atom__WIN_WORKSPACE)
			{
				taskbar_read_clientlist (tb);
				gui_draw_taskbar (tb);
			}
		}
		return;
	}

	tk = find_task (tb, win);
	if (!tk)
		return;

	if (at == XA_WM_NAME || at == atom__NET_WM_NAME)
	{
		/* window's title changed */
		if (tk->name)
			XFree (tk->name);
		tk->name = get_window_name (tk->win);
		gui_draw_task (tb, tk);
	} else if (at == atom_WM_STATE)
	{
		/* iconified state changed? */
		if (is_iconified (tk->win) != tk->iconified)
		{
			tk->iconified = !tk->iconified;
			gui_draw_task (tb, tk);
		}
	} else if (at == XA_WM_HINTS)
	{
		/* some windows set their WM_HINTS icon after mapping */
		if (tk->icon == generic_icon)
		{
			get_task_hinticon (tk);
			gui_draw_task (tb, tk);
		}
	}
}

void
handle_error (Display * d, XErrorEvent * ev)
{
}

int
#ifdef NOSTDLIB
_start (void)
#else
main (int argc, char *argv[])
#endif
{
	taskbar *tb;
	XEvent ev;
	fd_set fd;
	struct timeval tv;
	int xfd;
	int i;
	time_t now;
	struct tm *lt;
	void *prop;

	dd = XOpenDisplay (NULL);
	if (!dd)
		return 0;
	scr_screen = DefaultScreen (dd);
	scr_depth = DefaultDepth (dd, scr_screen);
	scr_height = DisplayHeight (dd, scr_screen);
	scr_width = DisplayWidth (dd, scr_screen);
	root_win = RootWindow (dd, scr_screen);

	for (i = 1; i < argc; ++i) {
		if (strcmp("--background", argv[i]) == 0) {
			always_on_top = 0;
		}
	}

	/* helps us catch windows closing/opening */
	XSelectInput (dd, root_win, PropertyChangeMask);

	XSetErrorHandler ((XErrorHandler) handle_error);

	XInternAtoms (dd, atom_names, ATOM_COUNT, False, atoms);

	/* check if the WM supports EWMH
	Note that this is not reliable. When switching to a EWMH-unaware WM, it
	will not delete this property. Also, we can't react to changes in this
	without a restart. */
	prop = get_prop_data (root_win, atom__NET_SUPPORTED, XA_ATOM, NULL);
	if (prop)
	{
		wm_use_ewmh = 1;
		XFree (prop);
	}

	gui_init ();
	tb = gui_create_taskbar ();
	xfd = ConnectionNumber (dd);
	gui_sync ();

	while (1)
	{
		now = time (0);
		lt = gmtime (&now);
		tv.tv_usec = 0;
		tv.tv_sec = 60 - lt->tm_sec;
		FD_ZERO (&fd);
		FD_SET (xfd, &fd);
		if (select (xfd + 1, &fd, 0, 0, &tv) == 0)
			gui_draw_clock (tb);

		while (XPending (dd))
		{
			XNextEvent (dd, &ev);
			switch (ev.type)
			{
			case ButtonPress:
				if (ev.xbutton.button == 1)
					handle_press (tb, ev.xbutton.x, ev.xbutton.y);
				break;
			case DestroyNotify:
				del_task (tb, ev.xdestroywindow.window);
				/* fall through */
			case Expose:
				gui_draw_taskbar (tb);
				break;
			case PropertyNotify:
				handle_propertynotify (tb, ev.xproperty.window,
											  ev.xproperty.atom);
				break;
			case FocusIn:
				handle_focusin (tb, ev.xfocus.window);
				break;
			/*default:
				   printf ("unknown evt type: %d\n", ev.type);*/
			}
		}
	}

	/*XCloseDisplay (dd);

	return 0;*/
}
