/* Copyright (C) 2020 David Brunecz. Subject to GPL 2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main(int argc, char *argv[])
{
	struct timeval tv;
	int num_ready_fds;
	fd_set in_fds;
	Display *dis;
	Window win;
	int x11_fd;
	XEvent ev;

	dis = XOpenDisplay(NULL);
	win = XCreateSimpleWindow(dis, RootWindow(dis, 0), 1, 1, 256, 256, 0,
				  BlackPixel(dis, 0), BlackPixel(dis, 0));

	XSelectInput(dis, win,
			ExposureMask | KeyPressMask | KeyReleaseMask |
			PointerMotionMask | ButtonPressMask |
			ButtonReleaseMask  | StructureNotifyMask);

	XMapWindow(dis, win);
	XFlush(dis);

	x11_fd = ConnectionNumber(dis);

	for ( ;; ) {
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		tv.tv_usec = 0;
		tv.tv_sec = 1;

		num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds > 0)
			printf("Event Received!\n");
		else if (num_ready_fds == 0)
			printf("Timer Fired!\n");
		else
			printf("An error occured!\n");

		for ( ;XPending(dis); )
			XNextEvent(dis, &ev);
	}
	return 0;
}
